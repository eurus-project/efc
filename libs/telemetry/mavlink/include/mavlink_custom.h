/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) (2026 - Present), The efc developers.
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

#ifndef __MAVLINK_CUSTOM_H__
#define __MAVLINK_CUSTOM_H__

#include "mavlink_types.h"

/* Prototypes must be visible to anyone including MAVLink */
mavlink_status_t *mavlink_get_channel_status(uint8_t chan);
mavlink_message_t *mavlink_get_channel_buffer(uint8_t chan);

#endif
