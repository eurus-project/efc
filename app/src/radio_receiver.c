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

#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "radio_receiver.h"

struct radio_receiver_data {
    int32_t roll_rate;
    int32_t pitch_rate;
    int32_t throttle;
    int32_t yaw_rate;
};

LOG_MODULE_REGISTER(radio_receiver);

static const struct device *const sbus_dev =
    DEVICE_DT_GET(DT_CHOSEN(futaba_sbus));
static struct radio_receiver_data receiver_data;

K_MSGQ_DEFINE(sbus_msgq, sizeof(struct radio_receiver_data), 10, 1);

static void sbus_event_callback(struct input_event *evt, void *user_data) {
    switch (evt->code) {
    case INPUT_ABS_RX:
        receiver_data.roll_rate = evt->value;
        break;
    case INPUT_ABS_RY:
        receiver_data.pitch_rate = evt->value;
        break;
    case INPUT_ABS_THROTTLE:
        receiver_data.throttle = evt->value;
        break;
    case INPUT_ABS_RZ:
        receiver_data.yaw_rate = evt->value;
        break;
    default:
        break;
    }

    if (evt->sync) {
        k_msgq_put(&sbus_msgq, &receiver_data, K_NO_WAIT);
    }
}

INPUT_CALLBACK_DEFINE(sbus_dev, sbus_event_callback, NULL);

void radio_receiver(void *dummy1, void *dummy2, void *dummy3) {
    ARG_UNUSED(dummy1);
    ARG_UNUSED(dummy2);
    ARG_UNUSED(dummy3);

    struct radio_receiver_data read_data;

    if (!device_is_ready(sbus_dev)) {
        LOG_ERR("Radio receiver not found, device operation will proceed "
                "without receiver!");
        return;
    }

    while (true) {
        k_msgq_get(&sbus_msgq, &read_data, K_FOREVER);

        LOG_INF("Received SBUS data:\n"
                "THR: %d, PITCH_RATE: %d, ROLL_RATE: %d, YAW_RATE: %d\n",
                read_data.throttle, read_data.pitch_rate, read_data.roll_rate,
                read_data.yaw_rate);
    }
}
