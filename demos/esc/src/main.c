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

#include <math.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "esc.h"

LOG_MODULE_REGISTER(main);

#define ESC_SPEED_CHANGE_MS 1000
#define ESC_ARMED_INDICATOR_MS 100

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

int main(void) {
    /*
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
    */

    if (!gpio_is_ready_dt(&fw_running_led)) {
        LOG_ERR("The firmware running LED is not ready");
        return 0;
    }

    int ret = gpio_pin_configure_dt(&fw_running_led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Could not configure the firmware running LED as output");
        return 0;
    }

    /* ESC Initialization */
    ESC_Error_Type status;
    ESC_Inst_Type esc1;
    ESC_Protocol_Type protocol;

#if CONFIG_ESC_PWM
    protocol = ESC_PWM;
#elif CONFIG_ESC_ONESHOT_125
    protocol = ESC_ONESHOT_125;
#elif CONFIG_ESC_ONESHOT_42
    protocol = ESC_ONESHOT_42;
#elif CONFIG_ESC_MULTISHOT
    protocol = ESC_MULTISHOT;
#endif

    const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm3));

    if (!device_is_ready(pwm_dev))
        return;

    status = ESC_Init(&esc1, pwm_dev, 1, protocol);
    if (status != ESC_SUCCESS)
        return 0;

    /* Arming procedure */
    printk("ESC Arming...\n");

    status = ESC_Arm(&esc1);
    if (status != ESC_SUCCESS) {
        return 0;
    } else {
        printk("ESC Armed.\n");
        for (int i = 0; i < 10; i++) {
            gpio_pin_toggle_dt(&fw_running_led);
            k_msleep(ESC_ARMED_INDICATOR_MS);
        }
    }

    while (1) {

        for (float i = 0.0f; i < 1.0f; i += 0.1f) {
            gpio_pin_toggle_dt(&fw_running_led);
            ESC_SetSpeed(&esc1, i);
            k_msleep(ESC_SPEED_CHANGE_MS);
        }
        for (float i = 1.0f; i > 0.0f; i -= 0.1f) {
            gpio_pin_toggle_dt(&fw_running_led);
            ESC_SetSpeed(&esc1, i);
            k_msleep(ESC_SPEED_CHANGE_MS);
        }
    }

    return 0;
}
