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

#ifndef UDP_LINK_H
#define UDP_LINK_H

#include <stddef.h>
#include <stdint.h>

#include <uv.h>

// Connected UDP: must send before the OS will deliver anything back.
struct udp_link {
    uv_udp_t handle;
    void (*recv_cb)(void *user_data, const uint8_t *data, size_t len);
    void (*error_cb)(void *user_data, int status);
    void *user_data;
};

int udp_link_init(struct udp_link *link, uv_loop_t *loop, const char *remote_ip,
                  int remote_port,
                  void (*recv_cb)(void *user_data, const uint8_t *data,
                                  size_t len),
                  void (*error_cb)(void *user_data, int status),
                  void *user_data);

int udp_link_send(struct udp_link *link, const uint8_t *data, size_t len);

#endif // UDP_LINK_H
