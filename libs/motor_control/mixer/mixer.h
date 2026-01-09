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

#ifndef MIXER_H
#define MIXER_H

#include "esc.h"
#include <stdio.h>
#include <zephyr/kernel.h>

#define MAX_MOTOR_INSTANCES 6

typedef enum {
    MIXER_SUCCESS = 0,
    MIXER_INIT_ERROR,
    MIXER_ESC_ERROR,
    MIXER_INVALID_CFG,
    MIXER_OUT_OF_BOUND_VALS,
} MIXER_Error_Type;

typedef enum {
    MIXER_UAV_CFG_QUADROTOR_X = 0,
    MIXER_UAV_CFG_QUADROTOR_CROSS,
    MIXER_UAV_CFG_HEXAROTOR_X,
    MIXER_UAV_CFG_HEXAROTOR_CROSS,
} MIXER_UAV_Cfg_Type;

typedef struct {
    float thrust; // 0.0 to 1.0
    float roll;   // -1.0 to 1.0
    float pitch;  // -1.0 to 1.0
    float yaw;    // -1.0 to 1.0
} MIXER_Input_Type;

typedef struct {
    float roll_factor;     // -1.0 to 1.0
    float pitch_factor;    // -1.0 to 1.0
    float yaw_factor;      // -1.0 to 1.0
    float throttle_factor; //  1.0
} MIXER_Motor_Factors_Type;

typedef struct {
    ESC_Inst_Type motor_arr[MAX_MOTOR_INSTANCES];
    MIXER_Motor_Factors_Type motor_factors[MAX_MOTOR_INSTANCES];
    float motor_outputs[MAX_MOTOR_INSTANCES];
    MIXER_UAV_Cfg_Type uav_config;
    uint8_t motor_instances;
    uint8_t max_motor_num;
    bool initialized;
} MIXER_Inst_Type;

MIXER_Error_Type MIXER_AddMotor(MIXER_Inst_Type *mixer, ESC_Inst_Type *esc);

MIXER_Error_Type MIXER_Init(MIXER_Inst_Type *mixer, MIXER_UAV_Cfg_Type uav_cfg);

MIXER_Error_Type MIXER_Execute(MIXER_Inst_Type *mixer,
                               const MIXER_Input_Type *mixer_in);

#endif
