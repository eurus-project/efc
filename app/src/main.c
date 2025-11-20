/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) (2024 - Present), The efc developers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/zbus/zbus.h>

#include "logger.h"
#include "radio_receiver.h"
#include "telemetry_packer.h"
#include "telemetry_sender.h"

#include "types.h"

LOG_MODULE_REGISTER(main);

ZBUS_OBS_DECLARE(logger_sub);
ZBUS_OBS_DECLARE(telemetry_packer_sub);

ZBUS_CHAN_DEFINE(imu_chan, struct imu_6dof_data, NULL, NULL,
                 ZBUS_OBSERVERS(logger_sub, telemetry_packer_sub), {0});

ZBUS_CHAN_DEFINE(baro_chan, struct baro_data, NULL, NULL,
                 ZBUS_OBSERVERS(logger_sub, telemetry_packer_sub), {0});

// This LED simply blinks at an interval, indicating visually that the firmware
// is running If anything causes the whole firmware to abort, it will be
// apparent without looking at log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct fs_littlefs main_fs;

struct fs_mount_t main_fs_mount = {
    .type = FS_LITTLEFS,
    .fs_data = &main_fs,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
    .storage_dev = "SD_SPI",
    .mnt_point = "/SD:",
};

int32_t boot_count = -1;

static struct nvs_fs nvs;
#define NVS_PARTITION spiflash_partition0
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define NVS_SECTOR_COUNT 3
#define NVS_BOOT_COUNT_ID 1

struct k_pipe telemetry_ground_pipe;
static uint8_t telemetry_ground_pipe_data[1024];

K_THREAD_STACK_DEFINE(radio_thread_stack, 1024);
static struct k_thread radio_thread;

K_THREAD_STACK_DEFINE(telemetry_packer_thread_stack, 2048);
static struct k_thread telemetry_packer_thread;

K_THREAD_STACK_DEFINE(telemetry_sender_thread_stack, 1024);
static struct k_thread telemetry_sender_thread;

K_THREAD_STACK_DEFINE(logger_thread_stack, 8192);
static struct k_thread logger_thread;

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static struct usbd_context *sample_usbd;

static int enable_usb_device_next(void) {
    int err;

    sample_usbd = sample_usbd_init_device(NULL);
    if (sample_usbd == NULL) {
        return -ENODEV;
    }

    err = usbd_enable(sample_usbd);
    if (err) {
        return err;
    }

    return 0;
}
#endif /* defined(CONFIG_USB_DEVICE_STACK_NEXT) */

static int process_imu(const struct device *dev) {
    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from IMU!");
        return ret;
    }

    const int64_t timestamp_ms = k_uptime_get();

    struct sensor_value accel[3];
    ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret < 0) {
        LOG_ERR("Could not get accelerometer data!");
        return ret;
    }

    struct sensor_value gyro[3];
    ret = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (ret < 0) {
        LOG_ERR("Could not get gyroscope data!");
        return ret;
    }

    struct sensor_value temperature;
    ret = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
    if (ret < 0) {
        LOG_WRN("Could not get IMU die temperature data!");
    }

    const struct imu_6dof_data msg = {
        .timestamp_us = timestamp_ms * 1000,
        .accel_mps2[0] = sensor_value_to_float(&accel[0]),
        .accel_mps2[1] = sensor_value_to_float(&accel[1]),
        .accel_mps2[2] = sensor_value_to_float(&accel[2]),
        .gyro_radps[0] = sensor_value_to_float(&gyro[0]),
        .gyro_radps[1] = sensor_value_to_float(&gyro[1]),
        .gyro_radps[2] = sensor_value_to_float(&gyro[2]),
        .temperature_degc = sensor_value_to_float(&temperature),
    };

    ret = zbus_chan_pub(&imu_chan, &msg, K_NO_WAIT);
    if (ret < 0 && ret != -EAGAIN && ret != -EBUSY) {
        LOG_ERR("Failed to send imu message on zbus!");
    }

    return 0;
}

#ifdef CONFIG_ICM4268X_TRIGGER
static void handle_icm42688p_drdy(const struct device *dev,
                                  const struct sensor_trigger *trig) {
    process_imu(dev);
}
#endif

static int process_baro(const struct device *dev) {
    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from barometer!");
        return ret;
    }

    const uint32_t timestamp_ms = k_uptime_get();

    struct sensor_value baro_temp;
    ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &baro_temp);
    if (ret < 0) {
        LOG_ERR("Could not get barometer temperature data!");
        return ret;
    }

    struct sensor_value baro_press;
    ret = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &baro_press);
    if (ret < 0) {
        LOG_ERR("Could not get barometer pressure data!");
        return ret;
    }

    const struct baro_data msg = {
        .timestamp_us = timestamp_ms * 1000,
        .temperature_degc = sensor_value_to_float(&baro_temp),
        .pressure_kpa = sensor_value_to_float(&baro_press),
    };

    ret = zbus_chan_pub(&baro_chan, &msg, K_NO_WAIT);
    if (ret < 0 && ret != -EAGAIN && ret != -EBUSY) {
        LOG_ERR("Failed to send baro message on zbus!");
    }

    return 0;
}

static int update_boot_count(struct nvs_fs *nvs) {
    ssize_t ret =
        nvs_read(nvs, NVS_BOOT_COUNT_ID, &boot_count, sizeof(boot_count));
    if (ret == sizeof(boot_count) && boot_count >= 0) { // Item found
        boot_count++; // Increment boot_count if found, if not write zero
    } else {
        // Try to initialize boot_count if it is first write to NVS
        boot_count = 0;
        ret =
            nvs_write(nvs, NVS_BOOT_COUNT_ID, &boot_count, sizeof(boot_count));
        if (ret < 0) {
            LOG_ERR("Error in writting initial boot_count in NVS: %d", ret);
            return ret;
        }

        ret = nvs_read(nvs, NVS_BOOT_COUNT_ID, &boot_count, sizeof(boot_count));
        if (ret == sizeof(boot_count) && boot_count == 0) {
            LOG_INF("NVS successfully initialized boot_count!");
        } else {
            LOG_ERR("NVS failed to initialize boot_count: %d", ret);
            return ret;
        }
    }

    LOG_INF("boot_count: ID = %d, Value = %d\n", NVS_BOOT_COUNT_ID, boot_count);
    ret = nvs_write(nvs, NVS_BOOT_COUNT_ID, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Error in writting initial boot_count!");
    }
    return ret;
}

int main(void) {
    if (DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart)) {
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
        if (enable_usb_device_next()) {
            return 0;
        }
#else
        if (usb_enable(NULL)) {
            return 0;
        }
#endif
    }

    if (!gpio_is_ready_dt(&fw_running_led)) {
        LOG_ERR("The firmware running LED is not ready");
        return 0;
    }

    int ret;
    ret = gpio_pin_configure_dt(&fw_running_led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Could not configure the firmware running LED as output");
        return 0;
    }

    bool main_imu_using_trigger = false;

#if CONFIG_APP_PRIMARY_IMU_MPU6050
    const struct device *const main_imu =
        DEVICE_DT_GET(DT_NODELABEL(imu_mpu6050));
#elif CONFIG_APP_PRIMARY_IMU_ICM42688P
    const struct device *const main_imu =
        DEVICE_DT_GET(DT_NODELABEL(imu_icm42688p));

#ifdef CONFIG_ICM4268X_TRIGGER
    struct sensor_trigger icm42688p_trigger = {.type = SENSOR_TRIG_DATA_READY,
                                               .chan = SENSOR_CHAN_ALL};

    ret =
        sensor_trigger_set(main_imu, &icm42688p_trigger, handle_icm42688p_drdy);
    if (ret < 0) {
        LOG_ERR("Could not configure IMU trigger.\n");
        return 0;
    }

    main_imu_using_trigger = true;
#endif
#else
    LOG_ERR("IMU Device not selected!");
    return 0;
#endif

    if (!device_is_ready(main_imu)) {
        LOG_ERR("Device %s is not ready\n", main_imu->name);
        return 0;
    }

    const struct device *const main_baro = DEVICE_DT_GET_ANY(bosch_bme280);

    if (!device_is_ready(main_baro)) {
        printk("Device %s is not ready\n", main_baro->name);
        return 0;
    }

    ret = fs_mount(&main_fs_mount);
    if (ret < 0) {
        LOG_ERR("Could not mount filesystem!\n");
    }

    // Configure NVS
    struct flash_pages_info nvs_info;

    nvs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(nvs.flash_device)) {
        LOG_ERR("Flash device not ready!");
        return 0;
    }

    nvs.offset = NVS_PARTITION_OFFSET;
    ret = flash_get_page_info_by_offs(nvs.flash_device, nvs.offset, &nvs_info);
    if (ret) {
        LOG_ERR("Unable to get page info, error: %d\n", ret);
        return 0;
    }

    nvs.sector_size = nvs_info.size;
    nvs.sector_count = NVS_SECTOR_COUNT;

    ret = nvs_mount(&nvs);
    if (ret) {
        LOG_ERR("Flash init failed, error: %d\n", ret);
        return 0;
    }

    ret = update_boot_count(&nvs);
    if (ret < 0) {
        LOG_ERR("Could not update boot count from NVS!");
    }

    k_pipe_init(&telemetry_ground_pipe, telemetry_ground_pipe_data,
                sizeof(telemetry_ground_pipe_data));

    k_thread_create(&radio_thread, radio_thread_stack,
                    K_THREAD_STACK_SIZEOF(radio_thread_stack), radio_receiver,
                    NULL, NULL, NULL, 0, 0, K_NO_WAIT);

    k_thread_create(&telemetry_sender_thread, telemetry_sender_thread_stack,
                    K_THREAD_STACK_SIZEOF(telemetry_sender_thread_stack),
                    telemetry_sender, NULL, NULL, NULL,
                    K_LOWEST_APPLICATION_THREAD_PRIO - 2, 0, K_NO_WAIT);

    k_thread_create(&telemetry_packer_thread, telemetry_packer_thread_stack,
                    K_THREAD_STACK_SIZEOF(telemetry_packer_thread_stack),
                    telemetry_packer, NULL, NULL, NULL,
                    K_LOWEST_APPLICATION_THREAD_PRIO - 1, 0, K_NO_WAIT);

    k_thread_create(&logger_thread, logger_thread_stack,
                    K_THREAD_STACK_SIZEOF(logger_thread_stack), logger, NULL,
                    NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

    while (1) {
        ret = gpio_pin_toggle_dt(&fw_running_led);
        if (ret < 0) {
            LOG_ERR("Could not toggle the firmware running LED");
            return 0;
        }

        if (!main_imu_using_trigger) {
            process_imu(main_imu);
        }

        process_baro(main_baro);

        k_msleep(1000);
    }

    return 0;
}
