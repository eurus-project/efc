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

#include <stdio.h>
#include <string.h>

#include <uv.h>

// MAVLink headers
// clang-format off
#include "mavlink_custom.h" // Needs to be included before any MAVLink header inclusion
#include "common/mavlink.h"
// clang-format on

#define TARGET_IP "127.0.0.1"
#define TARGET_PORT 14560

static uv_udp_t udp;
static uv_udp_send_t send_req;
static struct sockaddr_in target_addr;

const uint8_t telemetry_system_id = 0;
const uint8_t telemetry_component_id = MAV_COMP_ID_AUTOPILOT1;
const uint8_t telemetry_channel_ground = MAVLINK_COMM_0;

static mavlink_message_t mavlink_msg;
static uint8_t mavlink_ser_buf[MAVLINK_MAX_PACKET_LEN];

static void udp_cb(uv_udp_send_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "UDP send error: %s\n", uv_strerror(status));
    }
}

static void heartbeat_timer_cb(uv_timer_t *handle) {
    mavlink_msg_heartbeat_pack_chan(
        telemetry_system_id, telemetry_component_id, telemetry_channel_ground,
        &mavlink_msg, MAV_TYPE_GENERIC, MAV_AUTOPILOT_GENERIC,
        MAV_MODE_FLAG_MANUAL_INPUT_ENABLED, 0, MAV_STATE_ACTIVE);

    const uint16_t telemetry_msg_len =
        mavlink_msg_to_send_buffer(mavlink_ser_buf, &mavlink_msg);

    uv_buf_t buf = uv_buf_init((char *)mavlink_ser_buf, telemetry_msg_len);

    uv_udp_send(&send_req, &udp, &buf, 1, (const struct sockaddr *)&target_addr,
                udp_cb);
}

int main(void) {
    uv_loop_t *loop = uv_default_loop();

    uv_udp_init(loop, &udp);
    uv_ip4_addr(TARGET_IP, TARGET_PORT, &target_addr);

    uv_timer_t heartbeat_timer;
    uv_timer_init(loop, &heartbeat_timer);
    uv_timer_start(&heartbeat_timer, heartbeat_timer_cb, 0, 1000);

    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);

    return 0;
}
