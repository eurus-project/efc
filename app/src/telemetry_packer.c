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

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "common/mavlink.h"

#include "types.h"

LOG_MODULE_REGISTER(telemetry_packer);

ZBUS_CHAN_DECLARE(imu_chan);
ZBUS_CHAN_DECLARE(baro_chan);

ZBUS_SUBSCRIBER_DEFINE(telemetry_packer_sub, 16);

extern uint8_t telemetry_system_id;
extern uint8_t telemetry_component_id;
extern uint8_t telemetry_channel_ground;

static mavlink_message_t mavlink_msg;
static uint8_t mavlink_ser_buf[MAVLINK_MAX_PACKET_LEN];

extern struct k_pipe telemetry_ground_pipe;

void telemetry_packer(void *dummy1, void *dummy2, void *dummy3) {
    while (true) {
        const struct zbus_channel *chan;
        int ret = zbus_sub_wait(&telemetry_packer_sub, &chan, K_FOREVER);
        if (ret < 0) {
            LOG_ERR(
                "Could not wait on the telemetry packer subscriber, aborting.");
            return;
        }

        if (chan == &imu_chan) {
            struct imu_6dof_data msg;
            ret = zbus_chan_read(chan, &msg, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to read from logger subscriber!");
            }

            mavlink_msg_scaled_imu_pack_chan(
                telemetry_system_id, telemetry_component_id,
                telemetry_channel_ground, &mavlink_msg, msg.timestamp_us / 1000,
                (int16_t)(msg.accel_mps2[0] * 1000.0f),
                (int16_t)(msg.accel_mps2[1] * 1000.0f),
                (int16_t)(msg.accel_mps2[2] * 1000.0f),
                (int16_t)(msg.gyro_radps[0] * 1000.0f),
                (int16_t)(msg.gyro_radps[1] * 1000.0f),
                (int16_t)(msg.gyro_radps[2] * 1000.0f), 0, 0, 0,
                (int16_t)(msg.temperature_degc * 100.0f));

            const uint16_t telemetry_msg_len =
                mavlink_msg_to_send_buffer(mavlink_ser_buf, &mavlink_msg);

            ret = k_pipe_write(&telemetry_ground_pipe, mavlink_ser_buf,
                               telemetry_msg_len, K_NO_WAIT);

            if (ret < 0) {
                LOG_WRN("Could not fit data into telemetry pipe!");
            }

        } else if (chan == &baro_chan) {
            struct baro_data msg;
            ret = zbus_chan_read(chan, &msg, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to read from logger subscriber!");
            }

            mavlink_msg_scaled_pressure_pack_chan(
                telemetry_system_id, telemetry_component_id,
                telemetry_channel_ground, &mavlink_msg, msg.timestamp_us / 1000,
                msg.pressure_kpa * 10.0f, 0.0f,
                (int16_t)(msg.temperature_degc * 100.0f), 0);

            const uint16_t telemetry_msg_len =
                mavlink_msg_to_send_buffer(mavlink_ser_buf, &mavlink_msg);

            ret = k_pipe_write(&telemetry_ground_pipe, mavlink_ser_buf,
                               telemetry_msg_len, K_NO_WAIT);

            if (ret < 0) {
                LOG_WRN("Could not fit data into telemetry pipe!");
            }
        }
    }
}
