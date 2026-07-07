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

#include "udp_link.h"

#include <stdlib.h>
#include <string.h>

static void alloc_cb(uv_handle_t *handle, size_t suggested_size,
                     uv_buf_t *buf) {
    (void)handle;
    static char rx_buf[65536];
    buf->base = rx_buf;
    buf->len =
        suggested_size < sizeof(rx_buf) ? suggested_size : sizeof(rx_buf);
}

static void on_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                    const struct sockaddr *addr, unsigned flags) {
    (void)addr;
    (void)flags;
    struct udp_link *link = (struct udp_link *)handle->data;

    if (nread <= 0 || link->recv_cb == NULL) {
        return;
    }

    link->recv_cb(link->user_data, (const uint8_t *)buf->base, (size_t)nread);
}

static void on_send(uv_udp_send_t *req, int status) {
    if (status < 0) {
        struct udp_link *link = (struct udp_link *)req->handle->data;
        if (link->error_cb != NULL) {
            link->error_cb(link->user_data, status);
        }
    }
    free(req);
}

int udp_link_init(struct udp_link *link, uv_loop_t *loop, const char *remote_ip,
                  int remote_port,
                  void (*recv_cb_fn)(void *user_data, const uint8_t *data,
                                     size_t len),
                  void (*error_cb_fn)(void *user_data, int status),
                  void *user_data) {
    link->recv_cb = recv_cb_fn;
    link->error_cb = error_cb_fn;
    link->user_data = user_data;
    link->handle.data = link;

    int ret = uv_udp_init(loop, &link->handle);
    if (ret != 0) {
        return ret;
    }

    struct sockaddr_in remote_addr;
    ret = uv_ip4_addr(remote_ip, remote_port, &remote_addr);
    if (ret != 0) {
        return ret;
    }

    ret = uv_udp_connect(&link->handle, (const struct sockaddr *)&remote_addr);
    if (ret != 0) {
        return ret;
    }

    return uv_udp_recv_start(&link->handle, alloc_cb, on_recv);
}

// uv_udp_send() only queues the uv_buf_t descriptor, not the bytes it
// points to: if the write can't complete synchronously, libuv reads from
// that memory later on. The request and its payload are therefore one
// allocation, freed together once on_send() fires.
struct udp_send_req {
    uv_udp_send_t req;
    uint8_t data[];
};

int udp_link_send(struct udp_link *link, const uint8_t *data, size_t len) {
    struct udp_send_req *send_req = malloc(sizeof(*send_req) + len);
    if (send_req == NULL) {
        return -1;
    }
    memcpy(send_req->data, data, len);

    uv_buf_t buf = uv_buf_init((char *)send_req->data, len);
    int ret =
        uv_udp_send(&send_req->req, &link->handle, &buf, 1, NULL, on_send);
    if (ret != 0) {
        free(send_req);
    }

    return ret;
}
