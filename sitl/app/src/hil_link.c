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

#include "hil_link.h"

#include <stdio.h>

// MAVLink headers
// clang-format off
#include "mavlink_custom.h" // Needs to be included before any MAVLink header inclusion
#include "common/mavlink.h"
// clang-format on

#include "autopilot/autopilot.h"
#include "autopilot/types.h"
#include "gcs_link.h"

#define HIL_LINK_CHANNEL MAVLINK_COMM_1

// Every Nth HIL_SENSOR sample gets a verbose print, so the console stays
// readable at the simulator's ~250 Hz sensor rate.
#define VERBOSE_PRINT_PERIOD_SAMPLES 50

// A backward jump larger than this is treated as jMAVSim having restarted
// (its sim time resets near zero) rather than as UDP packet reordering.
#define SIM_TIME_RESET_THRESHOLD_US 1000000ULL

static uint64_t sim_time_us;

static void handle_hil_sensor(struct hil_link *link,
                              const mavlink_message_t *msg) {
    mavlink_hil_sensor_t hil_sensor;
    mavlink_msg_hil_sensor_decode(msg, &hil_sensor);

    if (hil_sensor.time_usec >= sim_time_us ||
        (sim_time_us - hil_sensor.time_usec) > SIM_TIME_RESET_THRESHOLD_US) {
        sim_time_us = hil_sensor.time_usec;
    }

    if (hil_sensor.fields_updated &
        (HIL_SENSOR_UPDATED_XACC | HIL_SENSOR_UPDATED_YACC |
         HIL_SENSOR_UPDATED_ZACC | HIL_SENSOR_UPDATED_XGYRO |
         HIL_SENSOR_UPDATED_YGYRO | HIL_SENSOR_UPDATED_ZGYRO)) {
        struct imu_6dof_data imu = {
            .timestamp_us = hil_sensor.time_usec,
            .temperature_degc = hil_sensor.temperature,
            .accel_mps2 = {hil_sensor.xacc, hil_sensor.yacc, hil_sensor.zacc},
            .gyro_radps = {hil_sensor.xgyro, hil_sensor.ygyro,
                           hil_sensor.zgyro},
        };
        autopilot_imu_input(&imu);
        if (link->gcs != NULL) {
            gcs_link_update_imu(link->gcs, &imu);
        }
    }

    if (hil_sensor.fields_updated & HIL_SENSOR_UPDATED_ABS_PRESSURE) {
        struct baro_data baro = {
            .timestamp_us = hil_sensor.time_usec,
            .temperature_degc = hil_sensor.temperature,
            .pressure_kpa = hil_sensor.abs_pressure / 10.0f, // hPa -> kPa
        };
        autopilot_baro_input(&baro);
        if (link->gcs != NULL) {
            gcs_link_update_baro(link->gcs, &baro);
        }
    }

    if (hil_sensor.fields_updated &
        (HIL_SENSOR_UPDATED_XMAG | HIL_SENSOR_UPDATED_YMAG |
         HIL_SENSOR_UPDATED_ZMAG)) {
        struct mag_data mag = {
            .timestamp_us = hil_sensor.time_usec,
            .mag_gauss = {hil_sensor.xmag, hil_sensor.ymag, hil_sensor.zmag},
        };
        autopilot_mag_input(&mag);
    }

    static uint32_t sample_count;
    if (link->verbose && (sample_count++ % VERBOSE_PRINT_PERIOD_SAMPLES) == 0) {
        printf("[hil] t=%llu accel=(%.2f,%.2f,%.2f) gyro=(%.2f,%.2f,%.2f) "
               "baro=%.2fkPa mag=(%.2f,%.2f,%.2f)\n",
               (unsigned long long)hil_sensor.time_usec, hil_sensor.xacc,
               hil_sensor.yacc, hil_sensor.zacc, hil_sensor.xgyro,
               hil_sensor.ygyro, hil_sensor.zgyro,
               hil_sensor.abs_pressure / 10.0f, hil_sensor.xmag,
               hil_sensor.ymag, hil_sensor.zmag);
    }

    struct actuator_outputs out;
    autopilot_step(sim_time_us, &out);

    mavlink_message_t out_msg;
    mavlink_msg_hil_actuator_controls_pack_chan(
        link->system_id, link->component_id, HIL_LINK_CHANNEL, &out_msg,
        out.timestamp_us, out.controls,
        out.armed ? MAV_MODE_FLAG_SAFETY_ARMED : 0, 0);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &out_msg);
    udp_link_send(&link->udp, buf, len);
}

static void handle_hil_gps(struct hil_link *link,
                           const mavlink_message_t *msg) {
    mavlink_hil_gps_t hil_gps;
    mavlink_msg_hil_gps_decode(msg, &hil_gps);

    struct gps_data gps = {
        .timestamp_us = hil_gps.time_usec,
        .fix_type = hil_gps.fix_type,
        .lat_dege7 = hil_gps.lat,
        .lon_dege7 = hil_gps.lon,
        .alt_mm = hil_gps.alt,
        .vel_n_mps = hil_gps.vn / 100.0f,
        .vel_e_mps = hil_gps.ve / 100.0f,
        .vel_d_mps = hil_gps.vd / 100.0f,
        .eph_m = hil_gps.eph / 100.0f,
        .epv_m = hil_gps.epv / 100.0f,
        .cog_deg = hil_gps.cog / 100.0f,
        .num_sats = hil_gps.satellites_visible,
    };
    autopilot_gps_input(&gps);
    if (link->gcs != NULL) {
        gcs_link_update_gps(link->gcs, &gps);
    }
}

static void log_unknown_message_once(uint32_t msgid) {
    // Common-dialect messages we might see all have msgid < 256; anything
    // outside that range would need a bigger table, but none of the
    // messages jMAVSim sends on this link do.
    static bool seen[256];
    if (msgid < 256 && !seen[msgid]) {
        seen[msgid] = true;
        printf("[hil] unhandled message id %u\n", msgid);
    }
}

static void on_udp_recv(void *user_data, const uint8_t *data, size_t len) {
    struct hil_link *link = (struct hil_link *)user_data;

    mavlink_message_t msg;
    mavlink_status_t status;
    for (size_t i = 0; i < len; i++) {
        if (!mavlink_parse_char(HIL_LINK_CHANNEL, data[i], &msg, &status)) {
            continue;
        }

        switch (msg.msgid) {
        case MAVLINK_MSG_ID_HIL_SENSOR:
            handle_hil_sensor(link, &msg);
            break;
        case MAVLINK_MSG_ID_HIL_GPS:
            handle_hil_gps(link, &msg);
            break;
        default:
            log_unknown_message_once(msg.msgid);
            break;
        }
    }
}

static void on_udp_error(void *user_data, int status) {
    (void)user_data;
    fprintf(stderr, "[hil] jMAVSim send failed, is it running? (%s)\n",
            uv_strerror(status));
}

int hil_link_init(struct hil_link *link, uv_loop_t *loop, const char *sim_ip,
                  int sim_port, uint8_t system_id, uint8_t component_id,
                  bool verbose, struct gcs_link *gcs) {
    link->system_id = system_id;
    link->component_id = component_id;
    link->verbose = verbose;
    link->gcs = gcs;

    return udp_link_init(&link->udp, loop, sim_ip, sim_port, on_udp_recv,
                         on_udp_error, link);
}

void hil_link_send_heartbeat(struct hil_link *link) {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack_chan(
        link->system_id, link->component_id, HIL_LINK_CHANNEL, &msg,
        MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_GENERIC, MAV_MODE_FLAG_HIL_ENABLED, 0,
        MAV_STATE_ACTIVE);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    udp_link_send(&link->udp, buf, len);
}
