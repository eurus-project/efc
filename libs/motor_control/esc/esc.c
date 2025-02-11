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

#include "esc.h"
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

// Default minimum pulse duration of ESC signals [us]
#define ESC_MIN_PULSE_PWM_US 1000
#define ESC_MIN_PULSE_ONESHOT_125_US 125
#define ESC_MIN_PULSE_ONESHOT_42_US 42
#define ESC_MIN_PULSE_MULTISHOT_US 5

// Default maximum pulse duration of ESC signals [us]
#define ESC_MAX_PULSE_PWM_US 2000
#define ESC_MAX_PULSE_ONESHOT_125_US 250
#define ESC_MAX_PULSE_ONESHOT_42_US 84
#define ESC_MAX_PULSE_MULTISHOT_US 25

//  ESC Protocol Period [us]
#define ESC_T_PWM_US 4000
#define ESC_T_ONESHOT_125 500
#define ESC_T_ONESHOT_42 168
#define ESC_T_MULTISHOT 50

// ESC Arming duty cycle [us]
#define ESC_ARM_PULSE_PWM_US 1000
#define ESC_ARM_PULSE_ONESHOT_125_US 125
#define ESC_ARM_PULSE_ONESHOT_42_US 42
#define ESC_ARM_PULSE_MULTISHOT_US 5
#define ESC_ARM_DURATION 3000 // NOTE: Arm duration is around 2000-5000 ms

static float ApplyMinOffset(float min_pulse_val_us, float max_pulse_val_us,
                            uint8_t offset);
static float ApplyMaxOffset(float min_pulse_val_us, float max_pulse_val_us,
                            uint8_t offset);

ESC_Error_Type ESC_Init(ESC_Inst_Type *esc_out, const struct device *pwm_dev,
                        const uint32_t pwm_channel,
                        const ESC_Protocol_Type protocol) {

    if (!device_is_ready(pwm_dev))
        return ESC_DEVICE_PWM_NOT_READY;

    switch (protocol) {
    // ESC PWM protocol config
    case ESC_PWM:
        esc_out->period_us = ESC_T_PWM_US;
#ifdef CONFIG_ESC_MANUAL_MIN_PULSE_OFFSET
        esc_out->min_pulse_duration_us =
            ApplyMinOffset(ESC_MIN_PULSE_PWM_US, ESC_MAX_PULSE_PWM_US,
                           CONFIG_ESC_MIN_PULSE_OFFSET_PERCENT);
#else
        esc_out->min_pulse_duration_us = ESC_MIN_PULSE_PWM_US;
#endif
#ifdef CONFIG_ESC_MANUAL_MAX_PULSE_OFFSET
        esc_out->max_pulse_duration_us =
            ApplyMaxOffset(ESC_MIN_PULSE_PWM_US, ESC_MAX_PULSE_PWM_US,
                           CONFIG_ESC_MAX_PULSE_OFFSET_VAL);
#else
        esc_out->max_pulse_duration_us = ESC_MAX_PULSE_PWM_US;
#endif
        esc_out->protocol = ESC_PWM;
        break;

    // ESC Oneshot125 protocol config
    case ESC_ONESHOT_125:
        esc_out->period_us = ESC_T_ONESHOT_125;
#ifdef CONFIG_ESC_MANUAL_MIN_PULSE_OFFSET
        esc_out->min_pulse_duration_us = ApplyMinOffset(
            ESC_MIN_PULSE_ONESHOT_125_US, ESC_MAX_PULSE_ONESHOT_125_US,
            CONFIG_ESC_MIN_PULSE_OFFSET_PERCENT);
#else
        esc_out->min_pulse_duration_us = ESC_MIN_PULSE_ONESHOT_125_US;
#endif
#ifdef CONFIG_ESC_MANUAL_MAX_PULSE_OFFSET
        esc_out->max_pulse_duration_us = ApplyMaxOffset(
            ESC_MIN_PULSE_ONESHOT_125_US, ESC_MAX_PULSE_ONESHOT_125_US,
            CONFIG_ESC_MAX_PULSE_OFFSET_PERCENT);
#else
        esc_out->max_pulse_duration_us = ESC_MAX_PULSE_ONESHOT_125_US;
#endif
        esc_out->protocol = ESC_ONESHOT_125;
        break;

    // ESC Oneshot42 protocol config
    case ESC_ONESHOT_42:
        esc_out->period_us = ESC_T_ONESHOT_42;
#ifdef CONFIG_ESC_MANUAL_MIN_PULSE_OFFSET
        esc_out->min_pulse_duration_us = ApplyMinOffset(
            ESC_MIN_PULSE_ONESHOT_42_US, ESC_MAX_PULSE_ONESHOT_42_US,
            CONFIG_ESC_MIN_PULSE_OFFSET_PERCENT);
#else
        esc_out->min_pulse_duration_us = ESC_MIN_PULSE_ONESHOT_42_US;
#endif
#ifdef CONFIG_ESC_MANUAL_MAX_PULSE_OFFSET
        esc_out->max_pulse_duration_us = ApplyMaxOffset(
            ESC_MIN_PULSE_ONESHOT_42_US, ESC_MAX_PULSE_ONESHOT_42_US,
            CONFIG_ESC_MAX_PULSE_OFFSET_PERCENT);
#else
        esc_out->max_pulse_duration_us = ESC_MAX_PULSE_ONESHOT_42_US;
#endif
        esc_out->protocol = ESC_ONESHOT_42;
        break;

    // ESC Multishot protocol config
    case ESC_MULTISHOT:
        esc_out->period_us = ESC_T_MULTISHOT;
#ifdef CONFIG_ESC_MANUAL_MIN_PULSE_OFFSET
        esc_out->min_pulse_duration_us = ApplyMinOffset(
            ESC_MIN_PULSE_MULTISHOT_US, ESC_MAX_PULSE_MULTISHOT_US,
            CONFIG_ESC_MIN_PULSE_OFFSET_PERCENT);
#else
        esc_out->min_pulse_duration_us = ESC_MIN_PULSE_MULTISHOT_US;
#endif
#ifdef CONFIG_ESC_MANUAL_MAX_PULSE_OFFSET
        esc_out->max_pulse_duration_us = ApplyMaxOffset(
            ESC_MIN_PULSE_MULTISHOT_US, ESC_MAX_PULSE_MULTISHOT_US,
            CONFIG_ESC_MAX_PULSE_OFFSET_PERCENT);
#else
        esc_out->max_pulse_duration_us = ESC_MAX_PULSE_MULTISHOT_US;
#endif
        esc_out->protocol = ESC_MULTISHOT;
        break;

    default:
        esc_out->initialized = false;
        return ESC_NOT_INITIALIZED;
    }
    esc_out->pwm_dev = pwm_dev;
    esc_out->pwm_channel = pwm_channel;
    esc_out->initialized = true;

    return ESC_SUCCESS;
}

ESC_Error_Type ESC_SetSpeed(const ESC_Inst_Type *esc, const float speed) {
    if (!esc->initialized) {
        return ESC_NOT_INITIALIZED;
    }

    if (speed > 1.0f || speed < 0.0f)
        return ESC_INVALID_SPEED_SET;

    uint32_t pulse_duration_us =
        (uint32_t)(((esc->max_pulse_duration_us - esc->min_pulse_duration_us) *
                    speed) +
                   (float)(esc->min_pulse_duration_us));

    int ret = pwm_set(esc->pwm_dev, esc->pwm_channel, PWM_USEC(esc->period_us),
                      PWM_USEC(pulse_duration_us), 0);

    if (ret) {
        return ESC_TIMER_OVERFLOW_ERROR;
    }

    return ESC_SUCCESS;
}

ESC_Error_Type ESC_Stop(const ESC_Inst_Type *esc) {
    if (!esc->initialized) {
        return ESC_NOT_INITIALIZED;
    }

    int ret = pwm_set(esc->pwm_dev, esc->pwm_channel, 0, 0, 0);
    if (ret) {
        return ESC_INVALID_STOP;
    }

    return ESC_SUCCESS;
}

ESC_Error_Type ESC_DeInit(ESC_Inst_Type *esc) {
    if (!esc->initialized) {
        return ESC_NOT_INITIALIZED;
    }

    ESC_Stop(esc);
    esc->initialized = false;

    return ESC_SUCCESS;
}

ESC_Error_Type ESC_Arm(const ESC_Inst_Type *esc) {
    uint32_t arm_duty_cycle;

    if (!esc->initialized) {
        return ESC_NOT_INITIALIZED;
    }

    switch (esc->protocol) {
    case ESC_PWM:
        arm_duty_cycle = ESC_ARM_PULSE_PWM_US;
        break;

    case ESC_ONESHOT_125:
        arm_duty_cycle = ESC_ARM_PULSE_ONESHOT_125_US;
        break;

    case ESC_ONESHOT_42:
        arm_duty_cycle = ESC_ARM_PULSE_ONESHOT_42_US;
        break;

    case ESC_MULTISHOT:
        arm_duty_cycle = ESC_ARM_PULSE_MULTISHOT_US;
        break;

    default:
        return ESC_INVALID_PROTOCOL;
    }

    int ret = pwm_set(esc->pwm_dev, esc->pwm_channel, PWM_USEC(esc->period_us),
                      PWM_USEC(arm_duty_cycle), 0);

    if (ret) {
        ESC_Stop(esc);
        return ESC_INVALID_ARMING;
    }

    k_msleep(ESC_ARM_DURATION);

    return ESC_SUCCESS;
}

static float ApplyMinOffset(float min_pulse_val_us, float max_pulse_val_us,
                            uint8_t offset) {
    return (min_pulse_val_us +
            ((max_pulse_val_us - min_pulse_val_us) * (offset / 100.0f)));
}

static float ApplyMaxOffset(float min_pulse_val_us, float max_pulse_val_us,
                            uint8_t offset) {
    return (max_pulse_val_us -
            ((max_pulse_val_us - min_pulse_val_us) * (offset / 100.0f)));
}
