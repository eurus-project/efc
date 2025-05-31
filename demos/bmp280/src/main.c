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

#include <math.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "types.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// This LED simply blinks at an interval, indicating visually that the firmware
// is running If anything causes the whole firmware to abort, it will be
// apparent without looking at log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

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

static int process_baro(const struct device *dev) {
    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from barometer!");
        return ret;
    }

    const uint32_t timestamp_ms = k_uptime_get();

    struct sensor_value baro_temp;
    ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &baro_temp);
    if (ret < 0) {
        LOG_ERR("Could not get barometer temperature data!");
        return ret;
    }

    struct sensor_value baro_press;
    ret = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &baro_press);
    if (ret < 0) {
        LOG_ERR("Could not get barometer pressure data!");
        return ret;
    }

    const struct baro_data msg = {
        .timestamp_us = timestamp_ms * 1000,
        .temperature_degc = sensor_value_to_float(&baro_temp),
        .pressure_kpa = sensor_value_to_float(&baro_press),
    };

    printk("Baro Data:\n");
    printk("  Timestamp: %llu us\n", msg.timestamp_us);
    printk("  Temperature: %.2f °C\n", msg.temperature_degc);
    printk("  Pressure: %.2f kPa\n", msg.pressure_kpa);

    return 0;
}

int main(void) {
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

    if (!gpio_is_ready_dt(&fw_running_led)) {
        LOG_ERR("The firmware running LED is not ready");
        return 0;
    }

    int ret;
    ret = gpio_pin_configure_dt(&fw_running_led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Could not configure the firmware running LED as output");
        return 0;
    }

    const struct device *const main_baro = DEVICE_DT_GET_ANY(bosch_bme280);

    if (!device_is_ready(main_baro)) {
        printk("Device %s is not ready\n", main_baro->name);
        return 0;
    }

    while (1) {
        ret = gpio_pin_toggle_dt(&fw_running_led);
        if (ret < 0) {
            LOG_ERR("Could not toggle the firmware running LED");
            return 0;
        }

        process_baro(main_baro);

        k_msleep(1000);
    }

    return 0;
}
