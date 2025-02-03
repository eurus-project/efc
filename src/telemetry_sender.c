/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024 - 2025 The efc developers.
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

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "telemetry_sender.h"

LOG_MODULE_REGISTER(telemetry_sender);

extern struct k_pipe telemetry_ground_pipe;
static const struct device *const ground_telemetry_uart =
    DEVICE_DT_GET(DT_CHOSEN(telemetry_ground));

void telemetry_sender(void *dummy1, void *dummy2, void *dummy3) {
    if (!device_is_ready(ground_telemetry_uart)) {
        LOG_ERR("Ground telemetry UART device not found, device operation will "
                "proceed without telemetry!");
        return;
    }

    while (true) {
        uint8_t read_byte;

        int ret = k_pipe_read(&telemetry_ground_pipe, &read_byte,
                              sizeof(read_byte), K_FOREVER);

        if (ret > 0) {
            uart_poll_out(ground_telemetry_uart, read_byte);
        } else {
            LOG_ERR("Could not read from telemetry pipe!");
        }
    }
}
