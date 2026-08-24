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
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "esc.h"
#include "mixer.h"

LOG_MODULE_REGISTER(mixer_demo, LOG_LEVEL_INF);

#define ESC_ARMED_INDICATOR_MS 100
#define MIXER_TIME_INCREMENT 10
#define AXIS_SWEEP_STEP 0.01f
#define THRUST_SWEEP_STEP 0.005f

// This LED simply blinks at an interval, indicating visually that the firmware
// is running If anything causes the whole firmware to abort, it will be
// apparent without looking at log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// Binds each quad X frame position to the PWM channel its ESC is wired to.
// The mixer output array is indexed by frame position, so this table is the
// single place where the physical wiring order is defined.
static const uint32_t motor_pwm_channel[MIXER_MAX_MOTORS] = {
    [MIXER_QUAD_X_MOTOR_FRONT_RIGHT] = 1,
    [MIXER_QUAD_X_MOTOR_REAR_LEFT] = 2,
    [MIXER_QUAD_X_MOTOR_FRONT_LEFT] = 3,
    [MIXER_QUAD_X_MOTOR_REAR_RIGHT] = 4,
};

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

// Reset control input to neutral/hover values
// Roll/Pitch/Yaw at 0.0 (neutral), Thrust at 0.5 (50% hover)
static void reset_control_input(MIXER_Input_Type *input) {
    input->roll = 0.0f;
    input->pitch = 0.0f;
    input->yaw = 0.0f;
    input->thrust = 0.5f;
}

// Mix the control input and forward the resulting motor outputs to the ESCs
static bool mix_and_apply(const MIXER_Inst_Type *mixer,
                          const ESC_Inst_Type *escs,
                          const MIXER_Input_Type *input) {
    MIXER_Output_Type output;

    if (MIXER_Calculate(mixer, input, &output) != MIXER_SUCCESS) {
        LOG_ERR("Mixer calculation failed");
        return false;
    }

    for (uint8_t i = 0; i < mixer->geometry->motor_count; i++) {
        if (ESC_SetSpeed(&escs[i], output.motor[i]) != ESC_SUCCESS) {
            LOG_ERR("Failed to set speed of motor %u", i);
            return false;
        }
    }

    return true;
}

// Sweep a single control axis from `from` to `to` in `step` increments,
// mixing and applying the motor outputs at every step
static bool sweep_axis(const MIXER_Inst_Type *mixer, const ESC_Inst_Type *escs,
                       const MIXER_Input_Type *input, float *axis, float from,
                       float to, float step) {
    const float increment = copysignf(step, to - from);
    const int num_steps = (int)(fabsf(to - from) / step);

    for (int i = 0; i <= num_steps; i++) {
        *axis = from + (float)i * increment;

        if (!mix_and_apply(mixer, escs, input)) {
            return false;
        }
        k_msleep(MIXER_TIME_INCREMENT);
    }

    return true;
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
    ESC_Inst_Type escs[MIXER_MAX_MOTORS];
    ESC_Protocol_Type protocol;

    /* MIXER variables */
    MIXER_Inst_Type mixer;
    MIXER_Input_Type control_input;

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

    const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm3));

    if (!device_is_ready(pwm_dev)) {
        return 0;
    }

    for (uint8_t i = 0; i < MIXER_MAX_MOTORS; i++) {
        if (ESC_Init(&escs[i], pwm_dev, motor_pwm_channel[i], protocol) !=
            ESC_SUCCESS) {
            LOG_ERR("Failed to initialize ESC on PWM channel %u",
                    motor_pwm_channel[i]);
            return 0;
        }
    }

    /* Arming procedure */
    LOG_INF("ESC Arming...");

    for (uint8_t i = 0; i < MIXER_MAX_MOTORS; i++) {
        if (ESC_Arm(&escs[i]) != ESC_SUCCESS) {
            LOG_ERR("Failed to arm ESC on PWM channel %u",
                    motor_pwm_channel[i]);
            return 0;
        }
    }

    LOG_INF("ESCs have been armed.");
    for (int i = 0; i < 10; i++) {
        gpio_pin_toggle_dt(&fw_running_led);
        k_msleep(ESC_ARMED_INDICATOR_MS);
    }

    /* Mixer initialization */
    if (MIXER_Init(&mixer, MIXER_FRAME_QUAD_X) != MIXER_SUCCESS) {
        LOG_ERR("Failed to initialize mixer");
        return 0;
    }

    while (1) {
        // Roll test: sweep to +1.0 (right), down to -1.0 (left) and back
        LOG_INF("Testing Roll...");
        reset_control_input(&control_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        if (!sweep_axis(&mixer, escs, &control_input, &control_input.roll, 0.0f,
                        1.0f, AXIS_SWEEP_STEP) ||
            !sweep_axis(&mixer, escs, &control_input, &control_input.roll, 1.0f,
                        -1.0f, AXIS_SWEEP_STEP) ||
            !sweep_axis(&mixer, escs, &control_input, &control_input.roll,
                        -1.0f, 0.0f, AXIS_SWEEP_STEP)) {
            return 0;
        }

        // Pitch test: sweep to +1.0 (forward), down to -1.0 (backward) and
        // back
        LOG_INF("Testing Pitch...");
        reset_control_input(&control_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        if (!sweep_axis(&mixer, escs, &control_input, &control_input.pitch,
                        0.0f, 1.0f, AXIS_SWEEP_STEP) ||
            !sweep_axis(&mixer, escs, &control_input, &control_input.pitch,
                        1.0f, -1.0f, AXIS_SWEEP_STEP) ||
            !sweep_axis(&mixer, escs, &control_input, &control_input.pitch,
                        -1.0f, 0.0f, AXIS_SWEEP_STEP)) {
            return 0;
        }

        // Yaw test: sweep to +1.0 (left), down to -1.0 (right) and back
        LOG_INF("Testing Yaw...");
        reset_control_input(&control_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        if (!sweep_axis(&mixer, escs, &control_input, &control_input.yaw, 0.0f,
                        1.0f, AXIS_SWEEP_STEP) ||
            !sweep_axis(&mixer, escs, &control_input, &control_input.yaw, 1.0f,
                        -1.0f, AXIS_SWEEP_STEP) ||
            !sweep_axis(&mixer, escs, &control_input, &control_input.yaw, -1.0f,
                        0.0f, AXIS_SWEEP_STEP)) {
            return 0;
        }

        // Thrust test: ramp from 0.0 (min) to 1.0 (max) and back
        LOG_INF("Testing Thrust...");
        reset_control_input(&control_input);
        k_msleep(MIXER_TIME_INCREMENT * 10);
        if (!sweep_axis(&mixer, escs, &control_input, &control_input.thrust,
                        0.0f, 1.0f, THRUST_SWEEP_STEP) ||
            !sweep_axis(&mixer, escs, &control_input, &control_input.thrust,
                        1.0f, 0.0f, THRUST_SWEEP_STEP)) {
            return 0;
        }
    }

    return 0;
}
