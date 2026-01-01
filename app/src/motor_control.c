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

#include "motor_control.h"
#include "esc.h"
#include "mixer.h"
#include "radio_receiver.h"
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_control);

extern struct k_msgq sbus_msgq;

void motor_control(void *dummy1, void *dummy2, void *dummy3) {
    struct radio_receiver_data receiver_data;

    ESC_Error_Type status;
    ESC_Inst_Type esc1, esc2, esc3, esc4;
    ESC_Protocol_Type protocol;

    MIXER_Inst_Type mixer;
    MIXER_UAV_Cfg_Type mixer_uav_geom_cfg;

    MIXER_Raw_Input_Type raw_receiver_input = {0};

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
        return 0;

    status = ESC_Init(&esc1, pwm_dev, 1, protocol);
    if (status != ESC_SUCCESS)
        return;
    status = ESC_Init(&esc2, pwm_dev, 2, protocol);
    if (status != ESC_SUCCESS)
        return;
    status = ESC_Init(&esc3, pwm_dev, 3, protocol);
    if (status != ESC_SUCCESS)
        return;
    status = ESC_Init(&esc4, pwm_dev, 4, protocol);
    if (status != ESC_SUCCESS)
        return;

    status = ESC_Arm(&esc1);
    if (status != ESC_SUCCESS)
        return;
    status = ESC_Arm(&esc2);
    if (status != ESC_SUCCESS)
        return;
    status = ESC_Arm(&esc3);
    if (status != ESC_SUCCESS)
        return;
    status = ESC_Arm(&esc4);
    if (status != ESC_SUCCESS)
        return;

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
        return;
    if (MIXER_AddMotorInstance(&mixer, &esc2) != MIXER_SUCCESS)
        return;
    if (MIXER_AddMotorInstance(&mixer, &esc3) != MIXER_SUCCESS)
        return;
    if (MIXER_AddMotorInstance(&mixer, &esc4) != MIXER_SUCCESS)
        return;

    if (MIXER_Init(&mixer, mixer_uav_geom_cfg) != MIXER_SUCCESS)
        return;

    while (true) {
        if (k_msgq_get(&sbus_msgq, &receiver_data, K_NO_WAIT) == 0) {
            raw_receiver_input.pitch = receiver_data.pitch_rate;
            raw_receiver_input.roll = receiver_data.roll_rate;
            raw_receiver_input.thrust = receiver_data.throttle;
            raw_receiver_input.yaw = receiver_data.yaw_rate;
        }

        // Update anyway
        MIXER_Execute(&mixer, &raw_receiver_input);

        k_msleep(50);
    }
}