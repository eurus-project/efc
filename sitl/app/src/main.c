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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>

// MAVLink headers
// clang-format off
#include "mavlink_custom.h" // Needs to be included before any MAVLink header inclusion
#include "common/mavlink.h"
// clang-format on

#include "autopilot/autopilot.h"
#include "gcs_link.h"
#include "hil_link.h"

#define DEFAULT_SIM_IP "127.0.0.1"
#define DEFAULT_SIM_PORT 14560
#define DEFAULT_GCS_IP "127.0.0.1"
#define DEFAULT_GCS_PORT 14550

#define SYSTEM_ID 1
#define COMPONENT_ID MAV_COMP_ID_AUTOPILOT1

static struct hil_link hil;
static struct gcs_link gcs;
static uv_timer_t heartbeat_timer;

static void heartbeat_timer_cb(uv_timer_t *handle) {
    (void)handle;
    hil_link_send_heartbeat(&hil);
    gcs_link_send_heartbeat(&gcs);
    gcs_link_send_sys_status(&gcs);
}

int main(int argc, char **argv) {
    // stdout is fully buffered when not a TTY (e.g. redirected to a file or
    // piped), which would otherwise delay --verbose sensor logs until the
    // buffer fills or the process exits cleanly.
    setvbuf(stdout, NULL, _IOLBF, 0);

    const char *sim_ip = DEFAULT_SIM_IP;
    int sim_port = DEFAULT_SIM_PORT;
    const char *gcs_ip = DEFAULT_GCS_IP;
    int gcs_port = DEFAULT_GCS_PORT;
    bool verbose = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sim-ip") == 0 && i + 1 < argc) {
            sim_ip = argv[++i];
        } else if (strcmp(argv[i], "--sim-port") == 0 && i + 1 < argc) {
            sim_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--gcs-ip") == 0 && i + 1 < argc) {
            gcs_ip = argv[++i];
        } else if (strcmp(argv[i], "--gcs-port") == 0 && i + 1 < argc) {
            gcs_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else {
            fprintf(stderr,
                    "Usage: %s [--sim-ip <ip>] [--sim-port <port>] [--gcs-ip "
                    "<ip>] [--gcs-port <port>] "
                    "[--verbose]\n",
                    argv[0]);
            return 1;
        }
    }

    autopilot_init();

    uv_loop_t *loop = uv_default_loop();

    int ret =
        gcs_link_init(&gcs, loop, gcs_ip, gcs_port, SYSTEM_ID, COMPONENT_ID);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize GCS link to %s:%d: %s\n", gcs_ip,
                gcs_port, uv_strerror(ret));
        return 1;
    }

    ret = hil_link_init(&hil, loop, sim_ip, sim_port, SYSTEM_ID, COMPONENT_ID,
                        verbose, &gcs);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize HIL link to %s:%d: %s\n", sim_ip,
                sim_port, uv_strerror(ret));
        return 1;
    }

    uv_timer_init(loop, &heartbeat_timer);
    uv_timer_start(&heartbeat_timer, heartbeat_timer_cb, 0, 1000);

    printf("efc_sitl: connecting to jMAVSim at %s:%d, sending telemetry to GCS "
           "at %s:%d\n",
           sim_ip, sim_port, gcs_ip, gcs_port);

    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);

    return 0;
}
