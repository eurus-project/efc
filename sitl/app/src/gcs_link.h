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

#ifndef GCS_LINK_H
#define GCS_LINK_H

#include <stdint.h>

#include <uv.h>

#include "autopilot/types.h"
#include "udp_link.h"

// Sends EFC's own telemetry, not jMAVSim's, so SITL exercises the same path
// the firmware uses on real hardware.
struct gcs_link {
    struct udp_link udp;
    uint8_t system_id;
    uint8_t component_id;
    uint64_t last_imu_send_us;
    uint64_t last_baro_send_us;
};

int gcs_link_init(struct gcs_link *link, uv_loop_t *loop, const char *gcs_ip,
                  int gcs_port, uint8_t system_id, uint8_t component_id);

void gcs_link_send_heartbeat(struct gcs_link *link);
void gcs_link_send_sys_status(struct gcs_link *link);

// Rate-limited to ~25 Hz internally; safe to call at full sensor rate.
void gcs_link_update_imu(struct gcs_link *link,
                         const struct imu_6dof_data *data);
void gcs_link_update_baro(struct gcs_link *link, const struct baro_data *data);
void gcs_link_update_gps(struct gcs_link *link, const struct gps_data *data);

#endif // GCS_LINK_H
