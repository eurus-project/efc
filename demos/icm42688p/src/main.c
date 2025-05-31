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

const struct device *const main_imu =
    DEVICE_DT_GET(DT_NODELABEL(imu_icm42688p));

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

static int process_imu(const struct device *dev) {
    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from IMU!");
        return ret;
    }

    const int64_t timestamp_ms = k_uptime_get();

    struct sensor_value accel[3];
    ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret < 0) {
        LOG_ERR("Could not get accelerometer data!");
        return ret;
    }

    struct sensor_value gyro[3];
    ret = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (ret < 0) {
        LOG_ERR("Could not get gyroscope data!");
        return ret;
    }

    struct sensor_value temperature;
    ret = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
    if (ret < 0) {
        LOG_WRN("Could not get IMU die temperature data!");
    }

    const struct imu_6dof_data msg = {
        .timestamp_us = timestamp_ms * 1000,
        .accel_mps2[0] = sensor_value_to_float(&accel[0]),
        .accel_mps2[1] = sensor_value_to_float(&accel[1]),
        .accel_mps2[2] = sensor_value_to_float(&accel[2]),
        .gyro_radps[0] = sensor_value_to_float(&gyro[0]),
        .gyro_radps[1] = sensor_value_to_float(&gyro[1]),
        .gyro_radps[2] = sensor_value_to_float(&gyro[2]),
        .temperature_degc = sensor_value_to_float(&temperature),
    };

    printk("IMU Data at %u us:\n", msg.timestamp_us);

    printk("  Acceleration [m/s^2]: X=%.3f, Y=%.3f, Z=%.3f\n",
           msg.accel_mps2[0], msg.accel_mps2[1], msg.accel_mps2[2]);

    printk("  Gyroscope [rad/s]:    X=%.3f, Y=%.3f, Z=%.3f\n",
           msg.gyro_radps[0], msg.gyro_radps[1], msg.gyro_radps[2]);

    printk("  Temperature [°C]:     %.2f\n", msg.temperature_degc);

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

    if (!device_is_ready(main_imu)) {
        LOG_ERR("Device %s is not ready\n", main_imu->name);
        return 0;
    }

    while (1) {
        ret = gpio_pin_toggle_dt(&fw_running_led);
        if (ret < 0) {
            LOG_ERR("Could not toggle the firmware running LED");
            return 0;
        }

        process_imu(main_imu);

        k_msleep(1000);
    }

    return 0;
}
