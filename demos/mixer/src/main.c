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
#include "mixer.h"

LOG_MODULE_REGISTER(main);

#define ESC_ARMED_INDICATOR_MS 100
#define MIXER_TIME_INCREMENT 2

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

static void reset_mixer_vals(MIXER_Raw_Input_Type *mixer) {
    mixer->pitch = 0;
    mixer->roll = 0;
    mixer->thrust = 0;
    mixer->yaw = 0;
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

    int ret = gpio_pin_configure_dt(&fw_running_led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Could not configure the firmware running LED as output");
        return 0;
    }

    /* ESC variables */
    ESC_Error_Type status;
    ESC_Inst_Type esc1, esc2, esc3, esc4;
    ESC_Protocol_Type protocol;

    /* MIXER variables */
    MIXER_Inst_Type mixer;
    MIXER_UAV_Cfg_Type mixer_uav_geom_cfg;

    /* Demo test variables */
    MIXER_Raw_Input_Type raw_receiver_input;
    raw_receiver_input.roll = 0;
    raw_receiver_input.pitch = 0;
    raw_receiver_input.yaw = 0;
    raw_receiver_input.thrust = 0;

    /* ESC Initialization */
#if CONFIG_ESC_PWM
    protocol = ESC_PWM;
#elif CONFIG_ESC_ONESHOT_125
    protocol = ESC_ONESHOT_125;
#elif CONFIG_ESC_ONESHOT_42
    protocol = ESC_ONESHOT_42;
#elif CONFIG_ESC_MULTISHOT
    protocol = ESC_MULTISHOT;
#endif

    const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm1));

    if (!device_is_ready(pwm_dev))
        return 0;

    status = ESC_Init(&esc1, pwm_dev, 1, protocol); // Motor 1 ESC
    if (status != ESC_SUCCESS)
        return 0;
    status = ESC_Init(&esc2, pwm_dev, 2, protocol); // Motor 2 ESC
    if (status != ESC_SUCCESS)
        return 0;
    status = ESC_Init(&esc3, pwm_dev, 3, protocol); // Motor 3 ESC
    if (status != ESC_SUCCESS)
        return 0;
    status = ESC_Init(&esc4, pwm_dev, 4, protocol); // Motor 4 ESC
    if (status != ESC_SUCCESS)
        return 0;

    /* Arming procedure */
    printk("ESC Arming...\n");

    status = ESC_Arm(&esc1); // Motor 1 ESC
    if (status != ESC_SUCCESS)
        return 0;
    status = ESC_Arm(&esc2); // Motor 2 ESC
    if (status != ESC_SUCCESS)
        return 0;
    status = ESC_Arm(&esc3); // Motor 3 ESC
    if (status != ESC_SUCCESS)
        return 0;
    status = ESC_Arm(&esc4); // Motor 4 ESC
    if (status != ESC_SUCCESS)
        return 0;

    printk("ESC's has been armed.\n");
    for (int i = 0; i < 10; i++) {
        gpio_pin_toggle_dt(&fw_running_led);
        k_msleep(ESC_ARMED_INDICATOR_MS);
    }

    /* Mixer initialization */

#if CONFIG_MIXER_UAV_QUADROTOR_X
    mixer_uav_geom_cfg = MIXER_UAV_CFG_QUADROTOR_X;
#elif CONFIG_MIXER_UAV_QUADROTOR_CROSS
    mixer_uav_geom_cfg = MIXER_UAV_CFG_QUADROTOR_CROSS;
#elif CONFIG_MIXER_UAV_HEXAROTOR_X
    mixer_uav_geom_cfg = MIXER_UAV_CFG_HEXAROTOR_X;
#elif CONFIG_MIXER_UAV_HEXAROTOR_CROSS
    mixer_uav_geom_cfg = MIXER_UAV_CFG_HEXAROTOR_CROSS;
#endif

    if (MIXER_AddMotorInstance(&mixer, &esc1) != MIXER_SUCCESS)
        return 0;
    if (MIXER_AddMotorInstance(&mixer, &esc2) != MIXER_SUCCESS)
        return 0;
    if (MIXER_AddMotorInstance(&mixer, &esc3) != MIXER_SUCCESS)
        return 0;
    if (MIXER_AddMotorInstance(&mixer, &esc4) != MIXER_SUCCESS)
        return 0;

    if (MIXER_Init(&mixer, mixer_uav_geom_cfg) != MIXER_SUCCESS)
        return 0;

    while (1) {
        // Roll
        printk("Roll angle:");
        reset_mixer_vals(&raw_receiver_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        for (int i = 0; i < 2047; i++) {
            raw_receiver_input.roll = i;
            MIXER_Execute(&mixer, &raw_receiver_input);
            k_msleep(MIXER_TIME_INCREMENT);
        }

        // Pitch
        printk("Pitch angle:");
        reset_mixer_vals(&raw_receiver_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        for (int i = 0; i < 2047; i++) {
            raw_receiver_input.pitch = i;
            MIXER_Execute(&mixer, &raw_receiver_input);
            k_msleep(MIXER_TIME_INCREMENT);
        }

        // Yaw
        printk("Yaw angle:");
        reset_mixer_vals(&raw_receiver_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        for (int i = 0; i < 2047; i++) {
            raw_receiver_input.yaw = i;
            MIXER_Execute(&mixer, &raw_receiver_input);
            k_msleep(MIXER_TIME_INCREMENT);
        }

        // Thrust
        printk("Thrust:");
        reset_mixer_vals(&raw_receiver_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        for (int i = 0; i < 2047; i++) {
            raw_receiver_input.thrust = i;
            MIXER_Execute(&mixer, &raw_receiver_input);
            k_msleep(MIXER_TIME_INCREMENT);
        }
    }

    return 0;
}
