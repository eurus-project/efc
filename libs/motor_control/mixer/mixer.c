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

#include "mixer.h"
#include <stdio.h>
#include <zephyr/kernel.h>

#define MIXER_RECEIVER_DEFAULT_MIN_VALUE 0.0f
#define MIXER_RECEIVER_DEFAULT_MAX_VALUE 2047.0f
#define MIXER_RECEIVER_DEFAULT_NEUTRAL_VALUE 1024.0f

static void normalize_stick_input(const MIXER_Raw_Input_Type *mixer_raw,
                                  MIXER_Mapped_Input_Type *mixer_mapped);

MIXER_Error_Type MIXER_AddMotorInstance(MIXER_Inst_Type *mixer,
                                        ESC_Inst_Type *esc) {
    if (!esc->initialized) {
        return MIXER_ESC_ERROR;
    }

    static uint8_t motor_count = 0;

    mixer->motor_arr[motor_count] = *esc;

    if (motor_count == 0) {
        mixer->motor_instances = 1;
    } else if (mixer->motor_instances < MAX_MOTOR_INSTANCES) {
        mixer->motor_instances++;
    } else {
        return MIXER_ESC_ERROR;
    }

    motor_count++;

    return MIXER_SUCCESS;
}

MIXER_Error_Type MIXER_Init(MIXER_Inst_Type *mixer,
                            MIXER_UAV_Cfg_Type uav_cfg) {
    /* Add necessary checks here ... */

    /* UAV geometrical configuration will be fixed to quadrotor X configuration
       in the first editions of this feature. uav_config will be used in future
       for support of multiple UAV geometrical configurations. */
    mixer->uav_config = uav_cfg;

    return MIXER_SUCCESS;
}

MIXER_Error_Type MIXER_Execute(MIXER_Inst_Type *mixer,
                               MIXER_Raw_Input_Type *mixer_raw) {
    /* Quadrotor X multirotor configuration fixed  */
    float m1 = 0, m2 = 0, m3 = 0, m4 = 0;
    ESC_Error_Type esc_status;
    MIXER_Mapped_Input_Type mixer_mapped;

    /* Remapping the raw values to ESC-compatible ones */
    // MapReceiverValues(mixer_raw, &mixer_mapped);
    normalize_stick_input(mixer_raw, &mixer_mapped);

    // TODO: Add checks here for edge-cases like R=P=0 and Y>T
    switch (mixer->uav_config) {
    case MIXER_UAV_CFG_QUADROTOR_X:
        m1 = mixer_mapped.thrust - mixer_mapped.roll + mixer_mapped.pitch +
             mixer_mapped.yaw;
        m2 = mixer_mapped.thrust + mixer_mapped.roll - mixer_mapped.pitch +
             mixer_mapped.yaw;
        m3 = mixer_mapped.thrust + mixer_mapped.roll + mixer_mapped.pitch -
             mixer_mapped.yaw;
        m4 = mixer_mapped.thrust - mixer_mapped.roll - mixer_mapped.pitch -
             mixer_mapped.yaw;
        break;

    default:
        break;
    }
    esc_status = ESC_SetSpeed(&mixer->motor_arr[0], m1);
    if (esc_status < 0)
        return MIXER_ESC_ERROR;
    esc_status = ESC_SetSpeed(&mixer->motor_arr[1], m2);
    if (esc_status < 0)
        return MIXER_ESC_ERROR;
    esc_status = ESC_SetSpeed(&mixer->motor_arr[2], m3);
    if (esc_status < 0)
        return MIXER_ESC_ERROR;
    esc_status = ESC_SetSpeed(&mixer->motor_arr[3], m4);
    if (esc_status < 0)
        return MIXER_ESC_ERROR;

    return ESC_SUCCESS;
}

static void normalize_stick_input(const MIXER_Raw_Input_Type *mixer_raw,
                                  MIXER_Mapped_Input_Type *mixer_mapped) {
    int32_t min_receiver_val, max_receiver_val, neutral_receiver_val;

#ifdef CONFIG_MIXER_MINIMAL_RECEIVER_DATA_OFFSET
    min_receiver_val = CONFIG_MIXER_MINIMAL_RECEIVER_DATA_VALUE;
#else
    min_receiver_val = MIXER_RECEIVER_DEFAULT_MIN_VALUE;
#endif

#ifdef CONFIG_MIXER_MAXIMAL_RECEIVER_DATA_OFFSET
    max_receiver_val = CONFIG_MIXER_MAXIMAL_RECEIVER_DATA_VALUE
#else
    max_receiver_val = MIXER_RECEIVER_DEFAULT_MAX_VALUE;
#endif

#ifdef CONFIG_MIXER_NEUTRAL_RECEIVER_DATA_OFFSET
        neutral_receiver_val = CONFIG_MIXER_NEUTRAL_RECEIVER_DATA_VALUE
#else
    neutral_receiver_val = MIXER_RECEIVER_DEFAULT_NEUTRAL_VALUE;
#endif

        /* Map raw roll values:
            [min_receiver_val, neutral_receiver_val] -> [-1.0, 0.0] and
            [neutral_receiver_val, max_receiver_val] -> [0.0, 1.0]
        */
        if (mixer_raw->roll < neutral_receiver_val) {
        mixer_mapped->roll = (float)(mixer_raw->roll - neutral_receiver_val) /
                             (float)(neutral_receiver_val - min_receiver_val);
    }
    else {
        mixer_mapped->roll = (float)(mixer_raw->roll - neutral_receiver_val) /
                             (float)(max_receiver_val - neutral_receiver_val);
    }

    // Clamp calculated values
    if (mixer_mapped->roll < -1.0f)
        mixer_mapped->roll = -1.0f;
    if (mixer_mapped->roll > 1.0f)
        mixer_mapped->roll = 1.0f;

    /* Map raw pitch values:
        [min_receiver_val, neutral_receiver_val] -> [-1.0, 0.0] and
        [neutral_receiver_val, max_receiver_val] -> [0.0, 1.0]
    */
    if (mixer_raw->pitch < neutral_receiver_val) {
        mixer_mapped->pitch = (float)(mixer_raw->pitch - neutral_receiver_val) /
                              (float)(neutral_receiver_val - min_receiver_val);
    } else {
        mixer_mapped->pitch = (float)(mixer_raw->pitch - neutral_receiver_val) /
                              (float)(max_receiver_val - neutral_receiver_val);
    }

    // Clamp calculated values
    if (mixer_mapped->pitch < -1.0f)
        mixer_mapped->pitch = -1.0f;
    if (mixer_mapped->pitch > 1.0f)
        mixer_mapped->pitch = 1.0f;

    /* Map raw yaw values:
        [min_receiver_val, neutral_receiver_val] -> [-1.0, 0.0] and
        [neutral_receiver_val, max_receiver_val] -> [0.0, 1.0]
    */
    if (mixer_raw->yaw < neutral_receiver_val) {
        mixer_mapped->yaw = (float)(mixer_raw->yaw - neutral_receiver_val) /
                            (float)(neutral_receiver_val - min_receiver_val);
    } else {
        mixer_mapped->yaw = (float)(mixer_raw->yaw - neutral_receiver_val) /
                            (float)(max_receiver_val - neutral_receiver_val);
    }

    // Clamp calculated values
    if (mixer_mapped->yaw < -1.0f)
        mixer_mapped->yaw = -1.0f;
    if (mixer_mapped->yaw > 1.0f)
        mixer_mapped->yaw = 1.0f;

    /* Map raw thrust values:
        [min_receiver_val, max_receiver_val] -> [0.0, 1.0]
    */
    mixer_mapped->thrust = (float)(mixer_raw->thrust - min_receiver_val) /
                           (float)(max_receiver_val - min_receiver_val);

    // Clamp calculated values
    if (mixer_mapped->thrust < -1.0f)
        mixer_mapped->thrust = -1.0f;
    if (mixer_mapped->thrust > 1.0f)
        mixer_mapped->thrust = 1.0f;
}