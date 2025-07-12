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

    int ret = gpio_pin_configure_dt(&fw_running_led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Could not configure the firmware running LED as output");
        return 0;
    }

    /* ESC Initialization */
    ESC_Error_Type status;
    ESC_Inst_Type esc1, esc2, esc3, esc4, esc5, esc6, esc7, esc8;
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

    const struct device *pwm_dev1 = DEVICE_DT_GET(DT_NODELABEL(pwm1));
    const struct device *pwm_dev2 = DEVICE_DT_GET(DT_NODELABEL(pwm2));
    const struct device *pwm_dev3 = DEVICE_DT_GET(DT_NODELABEL(pwm3));

    if (!device_is_ready(pwm_dev1))
        return;

    if (!device_is_ready(pwm_dev2))
        return;

    if (!device_is_ready(pwm_dev3))
        return;

    // ESC1 - Timer3 channel 1
    status = ESC_Init(&esc1, pwm_dev3, 1, protocol);
    if (status < 0)
        return;

    // ESC2 - Timer3 channel 2
    status = ESC_Init(&esc2, pwm_dev3, 2, protocol);
    if (status < 0)
        return;

    // ESC3 - Timer3 channel 3
    status = ESC_Init(&esc3, pwm_dev3, 3, protocol);
    if (status < 0)
        return;

    // ESC4 - Timer3 channel 4
    status = ESC_Init(&esc4, pwm_dev3, 4, protocol);
    if (status < 0)
        return;

    // ESC5 - Timer1 channel 2n
    status = ESC_Init(&esc5, pwm_dev1, 2, protocol);
    if (status < 0)
        return;

    // ESC6 - Timer1 channel 4n
    status = ESC_Init(&esc6, pwm_dev1, 4, protocol);
    if (status < 0)
        return;

    // ESC7 - Timer2 channel 1
    status = ESC_Init(&esc7, pwm_dev2, 1, protocol);
    if (status < 0)
        return;

    // ESC8 - Timer2 channel 4
    status = ESC_Init(&esc8, pwm_dev2, 4, protocol);
    if (status < 0)
        return;

    /* Arming procedure */
    printk("ESC Arming...\n");

    status = ESC_Arm(&esc1);
    status = ESC_Arm(&esc2);
    status = ESC_Arm(&esc3);
    status = ESC_Arm(&esc4);
    status = ESC_Arm(&esc5);
    status = ESC_Arm(&esc6);
    status = ESC_Arm(&esc7);
    status = ESC_Arm(&esc8);

    if (status < 0) {
        return;
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
            ESC_SetSpeed(&esc2, i);
            ESC_SetSpeed(&esc3, i);
            ESC_SetSpeed(&esc4, i);
            ESC_SetSpeed(&esc5, i);
            ESC_SetSpeed(&esc6, i);
            ESC_SetSpeed(&esc7, i);
            ESC_SetSpeed(&esc8, i);
            k_msleep(ESC_SPEED_CHANGE_MS);
        }
        for (float i = 1.0f; i > 0.0f; i -= 0.1f) {
            gpio_pin_toggle_dt(&fw_running_led);
            ESC_SetSpeed(&esc1, i);
            ESC_SetSpeed(&esc2, i);
            ESC_SetSpeed(&esc3, i);
            ESC_SetSpeed(&esc4, i);
            ESC_SetSpeed(&esc5, i);
            ESC_SetSpeed(&esc6, i);
            ESC_SetSpeed(&esc7, i);
            ESC_SetSpeed(&esc8, i);
            k_msleep(ESC_SPEED_CHANGE_MS);
        }
    }

    return 0;
}
