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
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(sbus_demo, LOG_LEVEL_INF);

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static struct usbd_context *sample_usbd;

static int enable_usb_device_next(void) {
    int err;

    sample_usbd = sample_usbd_init_device(NULL);
    if (sample_usbd == NULL) {
        return -ENODEV;
    }

    err = usbd_enable(sample_usbd);
    if (err) {
        return err;
    }

    return 0;
}
#endif /* defined(CONFIG_USB_DEVICE_STACK_NEXT) */

struct sbus_data {
    int32_t roll;
    int32_t pitch;
    int32_t throttle;
    int32_t yaw;
};

static const struct device *const sbus_dev =
    DEVICE_DT_GET(DT_CHOSEN(futaba_sbus));
static struct sbus_data current_data;

K_MSGQ_DEFINE(sbus_msgq, sizeof(struct sbus_data), 10, 1);

static void sbus_event_callback(struct input_event *evt, void *user_data) {
    ARG_UNUSED(user_data);

    switch (evt->code) {
    case INPUT_ABS_RX:
        current_data.roll = evt->value;
        break;
    case INPUT_ABS_RY:
        current_data.pitch = evt->value;
        break;
    case INPUT_ABS_THROTTLE:
        current_data.throttle = evt->value;
        break;
    case INPUT_ABS_RZ:
        current_data.yaw = evt->value;
        break;
    default:
        break;
    }

    if (evt->sync) {
        k_msgq_put(&sbus_msgq, &current_data, K_NO_WAIT);
    }
}

INPUT_CALLBACK_DEFINE(sbus_dev, sbus_event_callback, NULL);

int main(void) {
    struct sbus_data data;

    /* Initialize USB for logging */
    if (DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart)) {
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
        if (enable_usb_device_next()) {
            return 0;
        }
#else
        if (usb_enable(NULL)) {
            return 0;
        }
#endif
    }

    LOG_INF("SBUS Demo Started");

    if (!device_is_ready(sbus_dev)) {
        LOG_ERR("SBUS device not ready!");
        return -1;
    }

    LOG_INF("SBUS device ready, waiting for data...");

    while (true) {
        k_msgq_get(&sbus_msgq, &data, K_FOREVER);

        LOG_INF("SBUS: Roll=%5d | Pitch=%5d | Throttle=%5d | Yaw=%5d",
                data.roll, data.pitch, data.throttle, data.yaw);
    }

    return 0;
}
