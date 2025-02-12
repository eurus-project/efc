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
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "ulog.h"
#include "ulog_accel.h"
#include "ulog_altitude.h"
#include "ulog_baro.h"
#include "ulog_gyro.h"

#include "types.h"

LOG_MODULE_REGISTER(logger);

ZBUS_CHAN_DECLARE(imu_chan);
ZBUS_CHAN_DECLARE(baro_chan);

ZBUS_CHAN_DEFINE(sync_chan, bool, NULL, NULL, ZBUS_OBSERVERS(logger_sub), 0);

ZBUS_SUBSCRIBER_DEFINE(logger_sub, 16);

// Value derrived from BMP280 datasheet (page 15 of 49) for the given sensor
// configuration.
#define BMP280_ALTITUDE_VARIANCE_M .017f

#define ALTITUDE_SOURCE_TYPE_BARO 1

extern uint32_t boot_count;

extern struct fs_mount_t main_fs_mount;

static struct k_timer sync_timer;

static ULOG_Inst_Type ulog_log;

static uint16_t gyro_msg_id = 0;
static uint16_t accel_msg_id = 0;
static uint16_t baro_msg_id = 0;
static uint16_t baro_alt_msg_id = 0;

static void sync_notify(struct k_timer *timer_id) {
    int ret = zbus_chan_notify(&sync_chan, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Could not notify logger to sync!");
    }
}

void logger(void *dummy1, void *dummy2, void *dummy3) {
    char filename[LFS_NAME_MAX] = {0};

    snprintf(filename, sizeof(filename), "%s/log_%d.ulg",
             main_fs_mount.mnt_point, (int)boot_count);
    ULOG_Config_Type log_cfg = {
        .filename = filename,
    };

    if (ULOG_Init(&ulog_log, &log_cfg) != ULOG_SUCCESS) {
        LOG_ERR("Could not open log! Proceeding without logging.");
        return;
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

#if CONFIG_APP_PRIMARY_IMU_MPU6050
    const char main_imu_name_key[] = "char[7] main_imu_name";
    const char main_imu_name[] = "MPU6050";
#elif CONFIG_APP_PRIMARY_IMU_ICM42688P
    const char main_imu_name_key[] = "char[9] main_imu_name";
    const char main_imu_name[] = "ICM42688P";
#endif
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

    k_timer_init(&sync_timer, sync_notify, NULL);
    k_timer_start(&sync_timer, K_MSEC(CONFIG_APP_DATA_LOGGING_SYNC_INTERVAL),
                  K_MSEC(CONFIG_APP_DATA_LOGGING_SYNC_INTERVAL));

    while (true) {
        const struct zbus_channel *chan;
        int ret = zbus_sub_wait(&logger_sub, &chan, K_FOREVER);
        if (ret < 0) {
            LOG_ERR("Could not wait on the logger subscriber, aborting.");
            return;
        }

        if (chan == &imu_chan) {
            struct imu_6dof_data msg;
            ret = zbus_chan_read(chan, &msg, K_USEC(1));
            if (ret < 0) {
                LOG_ERR("Failed to read from logger subscriber!");
            }

            ULOG_Gyro_Type gyro_msg = {
                .timestamp = msg.timestamp_us,
                .x = msg.gyro_radps[0],
                .y = msg.gyro_radps[1],
                .z = msg.gyro_radps[2],
            };
            ULOG_Gyro_Write(&ulog_log, &gyro_msg, gyro_msg_id);

            ULOG_Accel_Type accel_msg = {
                .timestamp = msg.timestamp_us,
                .x = msg.accel_mps2[0],
                .y = msg.accel_mps2[1],
                .z = msg.accel_mps2[2],
            };
            ULOG_Accel_Write(&ulog_log, &accel_msg, accel_msg_id);

        } else if (chan == &baro_chan) {
            struct baro_data msg;
            ret = zbus_chan_read(chan, &msg, K_USEC(1));
            if (ret < 0) {
                LOG_ERR("Failed to read from logger subscriber!");
            }

            ULOG_Baro_Type baro_msg = {
                .timestamp = msg.timestamp_us,
                .temperature = msg.temperature_degc,
                .pressure = msg.pressure_kpa,
            };

            ULOG_Baro_Write(&ulog_log, &baro_msg, baro_msg_id);

            const float altitude =
                44330.0f *
                (1.0f - powf((msg.pressure_kpa / 101.325f), 1.0f / 5.255f));

            ULOG_Altitude_Type altitude_msg = {
                .timestamp = msg.timestamp_us,
                .source = ALTITUDE_SOURCE_TYPE_BARO,
                .altitude = altitude,
                .variance = BMP280_ALTITUDE_VARIANCE_M};

            ULOG_Altitude_Write(&ulog_log, &altitude_msg, baro_alt_msg_id);
        } else if (chan == &sync_chan) {
            ULOG_Sync(&ulog_log);
        }
    }
}
