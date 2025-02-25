/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024-2025 The efc developers.
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

/**
 * @file   esc.c
 * @brief  Zephyr Electronic Speed Controller ESC driver for BLDC motors
 * @author Eurus-project
 */

/******************************** Includes ************************************/
#include "esc.h"
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

/****************************** Static Functions ******************************/
static status_t prvESC_ChannelCheck(const struct device *pwm_dev,
                                    uint32_t pwm_channel);

/******************************** Functions ***********************************/

/* TODO: Retreive some info from device tree about pwm channels
 */
status_t ESC_Init(const struct device *pwm_dev, uint32_t pwm_channel,
                  esc_protocol_t protocol, esc_t *esc_out) {
    static esc_number_t esc_instance_num = 0;

    if (!device_is_ready(pwm_dev))
        return -1;

    switch (protocol) {
    case ESC_PWM:
        esc_out->period = T_PWM_US;
        esc_out->dc_min = DC_MIN_PWM_US;
        esc_out->dc_max = DC_MAX_PWM_US;
        esc_out->protocol = ESC_PWM;
        break;

    case ESC_ONESHOT_125:
        esc_out->period = T_ONESHOT_125;
        esc_out->dc_min = DC_MIN_ONESHOT_125_US;
        esc_out->dc_max = DC_MAX_ONESHOT_125_US;
        esc_out->protocol = ESC_ONESHOT_125;
        break;

    case ESC_ONESHOT_42:
        esc_out->period = T_ONESHOT_42;
        esc_out->dc_min = DC_MIN_ONESHOT_42_US;
        esc_out->dc_max = DC_MAX_ONESHOT_42_US;
        esc_out->protocol = ESC_ONESHOT_42;
        break;

    case ESC_MULTISHOT:
        esc_out->period = T_MULTISHOT;
        esc_out->dc_min = DC_MIN_MULTISHOT_US;
        esc_out->dc_max = DC_MAX_MULTISHOT_US;
        esc_out->protocol = ESC_MULTISHOT;
        break;

    default:
        esc_out->flag = ESC_UNINITIALIZED;
        return -1;
    }
    esc_out->instance.pwm_dev = pwm_dev;
    esc_out->instance.pwm_channel = pwm_channel;
    esc_out->instance.instance_num = esc_instance_num;
    esc_out->flag = ESC_INITIALIZED;
    esc_instance_num++;

    return 0;
}

status_t ESC_SetSpeed(esc_t *esc, uint8_t speed) {
    int ret;
    uint32_t dc_speed;

    if (esc->flag != ESC_INITIALIZED) {
        printk("[ESC]: Error: Uninitialized ESC cannot be started!");
        return -1;
    }

    if (speed > 100)
        speed = 100;

    dc_speed = (esc->dc_max - esc->dc_min) * speed / 100 + esc->dc_min;

    ret = pwm_set(esc->instance.pwm_dev, esc->instance.pwm_channel,
                  PWM_USEC(esc->period), PWM_USEC(dc_speed), 0);
    if (ret) {
        printk("[ESC]: Error: Selected PWM device cannot generate specified\
        ESC signal, timer value could be overflowed due to the low prescaler\
        value.");
        return -1;
    }

    return 0;
}

status_t ESC_Stop(esc_t *esc) {
    int ret;

    if (esc->flag != ESC_INITIALIZED) {
        printk("[ESC]: Error: Uninitialized ESC cannot be stopped!");
        return -1;
    }

    ret = pwm_set(esc->instance.pwm_dev, esc->instance.pwm_channel, 0, 0, 0);
    if (ret) {
        printk("[ESC]: Error: Selected PWM device cannot be stopped!\n");
        return -1;
    }

    return 0;
}

status_t ESC_DeInit(esc_t *esc) {
    if (esc->flag != ESC_INITIALIZED) {
        printk("[ESC]: Error: Specified ESC is already deinitialized\n");
        return -1;
    }

    ESC_Stop(esc);
    esc->flag = ESC_UNINITIALIZED;

    return 0;
}

status_t ESC_Arm(esc_t *esc) {
    int ret;

    if (esc->flag != ESC_INITIALIZED) {
        printk("[ESC]: Error: Uninitialized ESC cannot be armed!\n");
        return -1;
    }

    switch (esc->protocol) {
    case ESC_PWM:
        ret = pwm_set(esc->instance.pwm_dev, esc->instance.pwm_channel,
                      PWM_USEC(esc->period), PWM_USEC(ARM_DC_PWM_US), 0);
        if (ret) {
            printk(
                "[ESC]: Error: Selected device cannot be armed propertly!\n");
            ESC_Stop(esc);
            return -1;
        }

        k_msleep(ARM_DURATION);

        break;

    case ESC_ONESHOT_125:
        ret =
            pwm_set(esc->instance.pwm_dev, esc->instance.pwm_channel,
                    PWM_USEC(esc->period), PWM_USEC(ARM_DC_ONESHOT_125_US), 0);
        if (ret) {
            printk(
                "[ESC]: Error: Selected device cannot be armed propertly!\n");
            ESC_Stop(esc);
            return -1;
        }

        k_msleep(ARM_DURATION);

        break;

    case ESC_ONESHOT_42:
        ret = pwm_set(esc->instance.pwm_dev, esc->instance.pwm_channel,
                      PWM_USEC(esc->period), PWM_USEC(ARM_DC_ONESHOT_42_US), 0);
        if (ret) {
            printk(
                "[ESC]: Error: Selected device cannot be armed propertly!\n");
            ESC_Stop(esc);
            return -1;
        }

        k_msleep(ARM_DURATION);

        break;

    case ESC_MULTISHOT:
        ret = pwm_set(esc->instance.pwm_dev, esc->instance.pwm_channel,
                      PWM_USEC(esc->period), PWM_USEC(ARM_DC_MULTISHOT_US), 0);
        if (ret) {
            printk(
                "[ESC]: Error: Selected device cannot be armed propertly!\n");
            ESC_Stop(esc);
            return -1;
        }

        k_msleep(ARM_DURATION);

        break;

    default:
        return -1;
    }

    return 0;
}

/************************** Static Functions **********************************/
static int prvESC_ChannelCheck(const struct device *pwm_dev,
                               uint32_t pwm_channel) {
    // TODO: Implement this in future!
}
