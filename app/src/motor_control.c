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

#include "motor_control.h"
#include "radio_receiver.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_control);

extern struct k_msgq sbus_msgq;

void motor_control(void *dummy1, void *dummy2, void *dummy3) {
    struct radio_receiver_data receiver_data;

    while (true) {
        k_msgq_get(&sbus_msgq, &receiver_data, K_NO_WAIT);

        k_msleep(100);
    }
}