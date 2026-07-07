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

#include "gcs_link.h"

#include <math.h>
#include <stdio.h>

// MAVLink headers
// clang-format off
#include "mavlink_custom.h" // Needs to be included before any MAVLink header inclusion
#include "common/mavlink.h"
// clang-format on

#define GCS_LINK_CHANNEL MAVLINK_COMM_0
#define GCS_TELEMETRY_PERIOD_US                                                \
    40000ULL // ~25 Hz, matches the firmware's telemetry cadence
#define STANDARD_GRAVITY_MPS2 9.80665f

// Sensors provided by the jMAVSim HIL link
#define SENSOR_HEALTH_MASK                                                     \
    (MAV_SYS_STATUS_SENSOR_3D_GYRO | MAV_SYS_STATUS_SENSOR_3D_ACCEL |          \
     MAV_SYS_STATUS_SENSOR_3D_MAG | MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE |  \
     MAV_SYS_STATUS_SENSOR_GPS)

static void send_msg(struct gcs_link *link, mavlink_message_t *msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, msg);
    udp_link_send(&link->udp, buf, len);
}

static void on_udp_error(void *user_data, int status) {
    (void)user_data;
    fprintf(stderr, "[gcs] QGroundControl send failed, is it running? (%s)\n",
            uv_strerror(status));
}

int gcs_link_init(struct gcs_link *link, uv_loop_t *loop, const char *gcs_ip,
                  int gcs_port, uint8_t system_id, uint8_t component_id) {
    link->system_id = system_id;
    link->component_id = component_id;
    link->last_imu_send_us = 0;
    link->last_baro_send_us = 0;

    return udp_link_init(&link->udp, loop, gcs_ip, gcs_port, NULL, on_udp_error,
                         link);
}

void gcs_link_send_heartbeat(struct gcs_link *link) {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack_chan(
        link->system_id, link->component_id, GCS_LINK_CHANNEL, &msg,
        MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_GENERIC, MAV_MODE_FLAG_HIL_ENABLED, 0,
        MAV_STATE_ACTIVE);
    send_msg(link, &msg);
}

void gcs_link_send_sys_status(struct gcs_link *link) {
    mavlink_message_t msg;
    mavlink_msg_sys_status_pack_chan(
        link->system_id, link->component_id, GCS_LINK_CHANNEL, &msg,
        SENSOR_HEALTH_MASK, SENSOR_HEALTH_MASK, SENSOR_HEALTH_MASK, 0,
        UINT16_MAX, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    send_msg(link, &msg);
}

void gcs_link_update_imu(struct gcs_link *link,
                         const struct imu_6dof_data *data) {
    if (data->timestamp_us - link->last_imu_send_us < GCS_TELEMETRY_PERIOD_US) {
        return;
    }
    link->last_imu_send_us = data->timestamp_us;

    // xacc/yacc/zacc are in mG, not mm/s^2.
    const float accel_to_mg = 1000.0f / STANDARD_GRAVITY_MPS2;

    mavlink_message_t msg;
    mavlink_msg_scaled_imu_pack_chan(
        link->system_id, link->component_id, GCS_LINK_CHANNEL, &msg,
        data->timestamp_us / 1000, (int16_t)(data->accel_mps2[0] * accel_to_mg),
        (int16_t)(data->accel_mps2[1] * accel_to_mg),
        (int16_t)(data->accel_mps2[2] * accel_to_mg),
        (int16_t)(data->gyro_radps[0] * 1000.0f),
        (int16_t)(data->gyro_radps[1] * 1000.0f),
        (int16_t)(data->gyro_radps[2] * 1000.0f), 0, 0, 0,
        (int16_t)(data->temperature_degc * 100.0f));
    send_msg(link, &msg);
}

void gcs_link_update_baro(struct gcs_link *link, const struct baro_data *data) {
    if (data->timestamp_us - link->last_baro_send_us <
        GCS_TELEMETRY_PERIOD_US) {
        return;
    }
    link->last_baro_send_us = data->timestamp_us;

    mavlink_message_t msg;
    mavlink_msg_scaled_pressure_pack_chan(
        link->system_id, link->component_id, GCS_LINK_CHANNEL, &msg,
        data->timestamp_us / 1000, data->pressure_kpa * 10.0f, 0.0f,
        (int16_t)(data->temperature_degc * 100.0f), 0);
    send_msg(link, &msg);
}

void gcs_link_update_gps(struct gcs_link *link, const struct gps_data *data) {
    mavlink_message_t msg;

    const uint16_t ground_speed_cms =
        (uint16_t)(sqrtf(data->vel_n_mps * data->vel_n_mps +
                         data->vel_e_mps * data->vel_e_mps) *
                   100.0f);

    mavlink_msg_gps_raw_int_pack_chan(
        link->system_id, link->component_id, GCS_LINK_CHANNEL, &msg,
        data->timestamp_us, data->fix_type, data->lat_dege7, data->lon_dege7,
        data->alt_mm, (uint16_t)(data->eph_m * 100.0f),
        (uint16_t)(data->epv_m * 100.0f), ground_speed_cms,
        (uint16_t)(data->cog_deg * 100.0f), data->num_sats, data->alt_mm, 0, 0,
        0, 0, 0);
    send_msg(link, &msg);

    // No home position tracked yet, so relative_alt == alt_mm.
    mavlink_msg_global_position_int_pack_chan(
        link->system_id, link->component_id, GCS_LINK_CHANNEL, &msg,
        data->timestamp_us / 1000, data->lat_dege7, data->lon_dege7,
        data->alt_mm, data->alt_mm, (int16_t)(data->vel_n_mps * 100.0f),
        (int16_t)(data->vel_e_mps * 100.0f),
        (int16_t)(data->vel_d_mps * 100.0f), UINT16_MAX);
    send_msg(link, &msg);
}
