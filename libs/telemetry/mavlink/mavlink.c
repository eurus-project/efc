/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) (2025 - Present), The efc developers.
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

#include "mavlink_custom.h"

/* Global status and buffer instances for each channel */
static mavlink_status_t chan_statuses[MAVLINK_COMM_NUM_BUFFERS];
static mavlink_message_t chan_buffers[MAVLINK_COMM_NUM_BUFFERS];

mavlink_status_t *mavlink_get_channel_status(uint8_t chan) {
    return &chan_statuses[chan];
}

mavlink_message_t *mavlink_get_channel_buffer(uint8_t chan) {
    return &chan_buffers[chan];
}
