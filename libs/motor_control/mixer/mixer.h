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

#ifndef MIXER_H
#define MIXER_H

#include "esc.h"
#include <stdio.h>
#include <zephyr/kernel.h>

typedef enum {
    MIXER_SUCCESS = 0,
    MIXER_ESC_ERROR,
} MIXER_Error_Type;

typedef enum {
    MIXER_UAV_CFG_QUADROTOR_X = 0,
    MIXER_UAV_CFG_QUADROTOR_CROSS,
    MIXER_UAV_CFG_HEXAROTOR_X,
    MIXER_UAV_CFG_HEXAROTOR_CROSS
} MIXER_UAV_Cfg_Type;

typedef struct {
    int32_t thrust;
    int32_t roll;
    int32_t pitch;
    int32_t yaw;
} MIXER_Input_Type;

typedef struct {
    ESC_Inst_Type motor_arr[MAX_MOTOR_INSTANCES];
    uint8_t motor_instances;
    MIXER_UAV_Cfg_Type uav_config;
} MIXER_Inst_Type;

MIXER_Error_Type MIXER_AddMotorInstance(MIXER_Inst_Type *mixer,
                                        ESC_Inst_Type *esc);

MIXER_Error_Type MIXER_Init(MIXER_Inst_Type *mixer, MIXER_UAV_Cfg_Type uav_cfg);

MIXER_Error_Type MIXER_Execute(MIXER_Inst_Type *mixer,
                               MIXER_Input_Type *mixer_in);

#endif