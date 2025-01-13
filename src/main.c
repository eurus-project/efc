/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024 The efc developers.
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

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "ulog.h"
#include "ulog_accel.h"
#include "ulog_gyro.h"

LOG_MODULE_REGISTER(main);

// This LED simply blinks at an interval, indicating visually that the firmware
// is running If anything causes the whole firmware to abort, it will be
// apparent without looking at log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct fs_littlefs main_fs;

static struct fs_mount_t main_fs_mount = {
    .type = FS_LITTLEFS,
    .fs_data = &main_fs,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
    .storage_dev = "SD",
    .mnt_point = "/SD:",
};

static ULOG_Inst_Type ulog_log;
static uint16_t gyro_msg_id = 0;
static uint16_t accel_msg_id = 0;

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
    struct sensor_value temperature;
    struct sensor_value accel[3];
    struct sensor_value gyro[3];
    uint64_t currentTime;

    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from IMU!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret < 0) {
        LOG_ERR("Could not get accelerometer data!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (ret < 0) {
        LOG_ERR("Could not get gyroscope data!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
    if (ret < 0) {
        LOG_WRN("Could not get IMU die temperature data!");
    }

    printf("Temperature:%g Cel\n"
           "Accelerometer: %f %f %f m/s/s\n"
           "Gyroscope:  %f %f %f rad/s\n",
           sensor_value_to_double(&temperature),
           sensor_value_to_double(&accel[0]), sensor_value_to_double(&accel[1]),
           sensor_value_to_double(&accel[2]), sensor_value_to_double(&gyro[0]),
           sensor_value_to_double(&gyro[1]), sensor_value_to_double(&gyro[2]));

    currentTime = k_uptime_get() * 1000;

    ULOG_Gyro_Type gyro_msg = {
        .timestamp = currentTime,
        .x = sensor_value_to_float(&gyro[0]),
        .y = sensor_value_to_float(&gyro[1]),
        .z = sensor_value_to_float(&gyro[2]),
    };

    ULOG_Accel_Type accel_msg = {
        .timestamp = currentTime,
        .x = sensor_value_to_float(&accel[0]),
        .y = sensor_value_to_float(&accel[1]),
        .z = sensor_value_to_float(&accel[2]),
    };

    ULOG_Gyro_Write(&ulog_log, &gyro_msg, gyro_msg_id);
    ULOG_Accel_Write(&ulog_log, &accel_msg, accel_msg_id);

    return 0;
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

    const struct device *const main_imu = DEVICE_DT_GET(DT_NODELABEL(main_imu));

    if (!device_is_ready(main_imu)) {
        LOG_ERR("Device %s is not ready\n", main_imu->name);
        return 0;
    }

    ret = fs_mount(&main_fs_mount);
    if (ret < 0) {
        LOG_ERR("Could not mount filesystem!\n");
        return 0;
    }

    uint32_t boot_count = 0;
    struct fs_file_t boot_count_file;
    fs_file_t_init(&boot_count_file);

    char filename[LFS_NAME_MAX];
    snprintf(filename, sizeof(filename), "%s/boot_count",
             main_fs_mount.mnt_point);
    ret = fs_open(&boot_count_file, filename, FS_O_RDWR | FS_O_CREATE);
    if (ret < 0) {
        LOG_ERR("Could not open file!\n");
        return 0;
    }

    ret = fs_read(&boot_count_file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Could not read from file!\n");
        return 0;
    }

    ret = fs_seek(&boot_count_file, 0, FS_SEEK_SET);
    if (ret < 0) {
        LOG_ERR("Could not seek to beggining of the file!\n");
        return 0;
    }

    boot_count += 1;
    ret = fs_write(&boot_count_file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Could not write to file!\n");
        return 0;
    }

    ret = fs_close(&boot_count_file);
    if (ret < 0) {
        LOG_ERR("Could not close file!\n");
        return 0;
    }

    LOG_INF("Boot count: %d\n", (int)boot_count);

    memset(filename, 0, sizeof(filename));
    snprintf(filename, sizeof(filename), "%s/log_%d.ulg",
             main_fs_mount.mnt_point, (int)boot_count);
    ULOG_Config_Type log_cfg = {
        .filename = filename,
    };

    if (ULOG_Init(&ulog_log, &log_cfg) != ULOG_SUCCESS) {
        LOG_ERR("Could not open log!");
    }

    if (ULOG_Gyro_RegisterFormat(&ulog_log) != ULOG_SUCCESS) {
        LOG_ERR("Could not register ULOG gyro format!");
    }

    if (ULOG_Accel_RegisterFormat(&ulog_log) != ULOG_SUCCESS) {
        LOG_ERR("Could not register ULOG accel format!");
    }

    const char sys_name_key[] = "char[3] sys_name";
    const char sys_name[] = "EFC";
    if (ULOG_AddInfo(&ulog_log, sys_name_key, strlen(sys_name_key), sys_name,
                     strlen(sys_name))) {
        LOG_ERR("Could not system name info to the log!");
    }

    const char main_imu_name_key[] = "char[7] main_imu_name";
    const char main_imu_name[] = "MPU6050";
    if (ULOG_AddInfo(&ulog_log, main_imu_name_key, strlen(main_imu_name_key),
                     main_imu_name, strlen(main_imu_name))) {
        LOG_ERR("Could not main IMU info to the log!");
    }

    if (ULOG_StartDataPhase(&ulog_log) != ULOG_SUCCESS) {
        LOG_ERR("Could not start ULOG data phase!");
    }

    if (ULOG_Gyro_Subscribe(&ulog_log, 0, &gyro_msg_id) != ULOG_SUCCESS) {
        LOG_ERR("Could not subscribe ULOG log to gyro message!");
    }

    if (ULOG_Accel_Subscribe(&ulog_log, 0, &accel_msg_id) != ULOG_SUCCESS) {
        LOG_ERR("Could not subscribe ULOG log to accel message!");
    }

    while (1) {
        ret = gpio_pin_toggle_dt(&fw_running_led);
        if (ret < 0) {
            LOG_ERR("Could not toggle the firmware running LED");
            return 0;
        }

        process_imu(main_imu);

        ULOG_Sync(&ulog_log);

        k_msleep(1000);
    }

    return 0;
}
