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

#ifndef HIL_LINK_H
#define HIL_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include <uv.h>

#include "udp_link.h"

struct gcs_link;

struct hil_link {
    struct udp_link udp;
    uint8_t system_id;
    uint8_t component_id;
    bool verbose;
    struct gcs_link *gcs; // optional, forwards decoded samples if set
};

int hil_link_init(struct hil_link *link, uv_loop_t *loop, const char *sim_ip,
                  int sim_port, uint8_t system_id, uint8_t component_id,
                  bool verbose, struct gcs_link *gcs);

void hil_link_send_heartbeat(struct hil_link *link);

#endif // HIL_LINK_H
