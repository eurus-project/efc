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
 * @file   mixer.c
 * @brief  Library for motor mixing in multirotors.
 * @author eurus-projects
 */

/********************************** Includes **********************************/
#include "mixer.h"
#include <stdio.h>
#include <zephyr/kernel.h>

/********************************** Functions *********************************/
mix_status_t MIXER_AddMotorInstance(esc_t *esc, mixer_t *mixer) {
    if (esc->flag != ESC_INITIALIZED) {
        printk("Error(mixer): Cannot add motor instance, esc not initialized!");
        return -1;
    }

    static uint8_t motor_count = 0;

    mixer->motor_arr[motor_count] = *esc;

    if (motor_count == 0) {
        mixer->motor_instances = 1;
    } else if (mixer->motor_instances < MAX_MOTOR_INSTANCES) {
        mixer->motor_instances++;
    } else {
        printk(
            "Error(mixer): Cannot exceed maximum number of motor instances!");
        return -1;
    }

    motor_count++;

    return 0;
}

mix_status_t MIXER_Init(mixer_t *mixer, mixer_uav_cfg_t uav_cfg) {
    /* Add necessary checks here ... */

    /* UAV geometrical configuration will be fixed to quadrotor X configuration
       in the first editions of this feature. uav_config will be used in future
       for support of multiple UAV geometrical configurations. */
    mixer->uav_config = uav_cfg;

    return 0;
}

mix_status_t MIXER_Execute(mixer_t *mixer, mixer_input_t *mix_in) {
    /* Quadrotor X multirotor configuration fixed  */
    uint8_t m1 = 0, m2 = 0, m3 = 0, m4 = 0;
    status_t esc_status;

    // TODO: Add checks here for edge-cases like R=P=0 and Y>T
    switch (mixer->uav_config) {
    case MIXER_UAV_CFG_QUADROTOR_X:
        m1 = mix_in->thrust - mix_in->roll + mix_in->pitch + mix_in->yaw;
        m2 = mix_in->thrust + mix_in->roll - mix_in->pitch + mix_in->yaw;
        m3 = mix_in->thrust + mix_in->roll + mix_in->pitch - mix_in->yaw;
        m4 = mix_in->thrust - mix_in->roll - mix_in->pitch - mix_in->yaw;
        break;

    default:
        break;
    }
    /* Set calculated motor speeds */
    esc_status = ESC_SetSpeed(&mixer->motor_arr[0], m1);
    if (esc_status < 0)
        return -1;
    esc_status = ESC_SetSpeed(&mixer->motor_arr[1], m2);
    if (esc_status < 0)
        return -1;
    esc_status = ESC_SetSpeed(&mixer->motor_arr[2], m3);
    if (esc_status < 0)
        return -1;
    esc_status = ESC_SetSpeed(&mixer->motor_arr[3], m4);
    if (esc_status < 0)
        return -1;

    return 0;
}
