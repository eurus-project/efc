/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024 - 2025 The efc developers.
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
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "common/mavlink.h"
#include "ulog.h"
#include "ulog_accel.h"
#include "ulog_altitude.h"
#include "ulog_baro.h"
#include "ulog_gyro.h"

#include "telemetry_sender.h"

#define ALTITUDE_SOURCE_TYPE_BARO 1

// Value derrived from BMP280 datasheet (page 15 of 49) for the given sensor
// configuration.
#define BMP280_ALTITUDE_VARIANCE_M .017f

LOG_MODULE_REGISTER(main);

// This LED simply blinks at an interval, indicating visually that the firmware
// is running If anything causes the whole firmware to abort, it will be
// apparent without looking at log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const uint8_t telemetry_system_id = 0;
static const uint8_t telemetry_component_id = MAV_COMP_ID_AUTOPILOT1;
static const uint8_t telemetry_channel_ground = 0;

static struct fs_littlefs main_fs;

static struct fs_mount_t main_fs_mount = {
    .type = FS_LITTLEFS,
    .fs_data = &main_fs,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
    .storage_dev = "SD",
    .mnt_point = "/SD:",
};

struct k_pipe telemetry_ground_pipe;
static uint8_t telemetry_ground_pipe_data[1024];

static mavlink_message_t mavlink_msg_buf;

static ULOG_Inst_Type ulog_log;
static uint16_t gyro_msg_id = 0;
static uint16_t accel_msg_id = 0;
static uint16_t baro_msg_id = 0;
static uint16_t baro_alt_msg_id = 0;

K_THREAD_STACK_DEFINE(telemetry_thread_stack, 2048);
static struct k_thread telemetry_thread;

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

    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from IMU!");
        return ret;
    }

    const int64_t timestamp_ms = k_uptime_get();

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

    const uint16_t telemetry_msg_len = mavlink_msg_scaled_imu_pack_chan(
        telemetry_system_id, telemetry_component_id, telemetry_channel_ground,
        &mavlink_msg_buf, timestamp_ms, sensor_value_to_milli(&accel[0]),
        sensor_value_to_milli(&accel[1]), sensor_value_to_milli(&accel[2]),
        sensor_value_to_milli(&gyro[0]), sensor_value_to_milli(&gyro[1]),
        sensor_value_to_milli(&gyro[2]), 0, 0, 0,
        sensor_value_to_milli(&temperature) / 10);

    ret =
        k_pipe_write(&telemetry_ground_pipe, (const uint8_t *)&mavlink_msg_buf,
                     telemetry_msg_len, K_NO_WAIT);

    if (ret < 0) {
        LOG_WRN("Could not fit data into telemetry pipe!");
    }

    ULOG_Gyro_Type gyro_msg = {
        .timestamp = timestamp_ms * 1000,
        .x = sensor_value_to_float(&gyro[0]),
        .y = sensor_value_to_float(&gyro[1]),
        .z = sensor_value_to_float(&gyro[2]),
    };

    ULOG_Accel_Type accel_msg = {
        .timestamp = timestamp_ms * 1000,
        .x = sensor_value_to_float(&accel[0]),
        .y = sensor_value_to_float(&accel[1]),
        .z = sensor_value_to_float(&accel[2]),
    };

    ULOG_Gyro_Write(&ulog_log, &gyro_msg, gyro_msg_id);
    ULOG_Accel_Write(&ulog_log, &accel_msg, accel_msg_id);

    return 0;
}

static int process_baro(const struct device *dev) {
    struct sensor_value baro_temp;
    struct sensor_value baro_press;

    float temperature;
    float pressure;
    float altitude;

    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from barometer!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &baro_temp);
    if (ret < 0) {
        LOG_ERR("Could not get barometer temperature data!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &baro_press);
    if (ret < 0) {
        LOG_ERR("Could not get barometer pressure data!");
        return ret;
    }

    temperature = sensor_value_to_float(&baro_temp);
    pressure = sensor_value_to_float(&baro_press);
    const uint32_t timestamp_ms = k_uptime_get();

    ULOG_Baro_Type baro_msg = {
        .timestamp = timestamp_ms * 1000,
        .temperature = temperature,
        .pressure = pressure,
    };

    const uint16_t telemetry_msg_len = mavlink_msg_scaled_pressure_pack_chan(
        telemetry_system_id, telemetry_component_id, telemetry_channel_ground,
        &mavlink_msg_buf, timestamp_ms, pressure * 10.0f, 0.0f,
        sensor_value_to_milli(&baro_temp) / 10, 0);

    ret =
        k_pipe_write(&telemetry_ground_pipe, (const uint8_t *)&mavlink_msg_buf,
                     telemetry_msg_len, K_NO_WAIT);

    if (ret < 0) {
        LOG_WRN("Could not fit data into telemetry pipe!");
    }

    ULOG_Baro_Write(&ulog_log, &baro_msg, baro_msg_id);

    altitude = 44330.0f * (1.0f - powf((pressure / 101.325f), 1.0f / 5.255f));

    ULOG_Altitude_Type altitude_msg = {.timestamp = timestamp_ms * 1000,
                                       .source = ALTITUDE_SOURCE_TYPE_BARO,
                                       .altitude = altitude,
                                       .variance = BMP280_ALTITUDE_VARIANCE_M};

    ULOG_Altitude_Write(&ulog_log, &altitude_msg, baro_alt_msg_id);

    return 0;
}

void telemetry_heartbeat(void) {
    LOG_INF("Sending heartbeat!");

    const uint16_t telemetry_msg_len = mavlink_msg_heartbeat_pack_chan(
        telemetry_system_id, telemetry_component_id, telemetry_channel_ground,
        &mavlink_msg_buf, MAV_TYPE_GENERIC, MAV_AUTOPILOT_GENERIC,
        MAV_MODE_FLAG_MANUAL_INPUT_ENABLED, 0, MAV_STATE_ACTIVE);

    int ret =
        k_pipe_write(&telemetry_ground_pipe, (const uint8_t *)&mavlink_msg_buf,
                     telemetry_msg_len, K_NO_WAIT);

    if (ret < 0) {
        LOG_WRN("Could not fit data into telemetry pipe!");
    }
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

    const struct device *const main_baro = DEVICE_DT_GET_ANY(bosch_bme280);

    if (!device_is_ready(main_baro)) {
        printk("Device %s is not ready\n", main_baro->name);
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

    k_pipe_init(&telemetry_ground_pipe, telemetry_ground_pipe_data,
                sizeof(telemetry_ground_pipe_data));

    k_thread_create(&telemetry_thread, telemetry_thread_stack,
                    K_THREAD_STACK_SIZEOF(telemetry_thread_stack),
                    telemetry_sender, NULL, NULL, NULL,
                    K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
    k_thread_start(&telemetry_thread);

    if (ULOG_Init(&ulog_log, &log_cfg) != ULOG_SUCCESS) {
        LOG_ERR("Could not open log!");
    }

    if (ULOG_Gyro_RegisterFormat(&ulog_log) != ULOG_SUCCESS) {
        LOG_ERR("Could not register ULOG gyro format!");
    }

    if (ULOG_Accel_RegisterFormat(&ulog_log) != ULOG_SUCCESS) {
        LOG_ERR("Could not register ULOG accel format!");
    }

    if (ULOG_Baro_RegisterFormat(&ulog_log) != ULOG_SUCCESS) {
        LOG_ERR("Could not register ULOG baro format!");
    }

    if (ULOG_Altitude_RegisterFormat(&ulog_log) != ULOG_SUCCESS) {
        LOG_ERR("Could not register ULOG altitude format!");
    }

    const char alt_src_type_baro_key[] = "int32_t ALTITUDE_SOURCE_TYPE_BARO";
    const int32_t alt_src_type_baro = ALTITUDE_SOURCE_TYPE_BARO;
    if (ULOG_AddParameter(&ulog_log, alt_src_type_baro_key,
                          strlen(alt_src_type_baro_key), &alt_src_type_baro)) {
        LOG_ERR("Could not write Altitude Source Type Baro parameter!");
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

    const char main_baro_name_key[] = "char[6] main_baro_name";
    const char main_baro_name[] = "BMP280";
    if (ULOG_AddInfo(&ulog_log, main_baro_name_key, strlen(main_baro_name_key),
                     main_baro_name, strlen(main_baro_name))) {
        LOG_ERR("Could not write main baro info to the log!");
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

    if (ULOG_Baro_Subscribe(&ulog_log, 0, &baro_msg_id) != ULOG_SUCCESS) {
        LOG_ERR("Could not subscribe ULog baro message!");
    }

    if (ULOG_Altitude_Subscribe(&ulog_log, 0, &baro_alt_msg_id) !=
        ULOG_SUCCESS) {
        LOG_ERR("Could not subscribe ULog altitude message!");
    }

    while (1) {
        ret = gpio_pin_toggle_dt(&fw_running_led);
        if (ret < 0) {
            LOG_ERR("Could not toggle the firmware running LED");
            return 0;
        }

        process_imu(main_imu);

        process_baro(main_baro);

        ULOG_Sync(&ulog_log);

        telemetry_heartbeat();

        k_msleep(1000);
    }

    return 0;
}
