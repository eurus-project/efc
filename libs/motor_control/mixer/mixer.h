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

/********************************** Includes **********************************/
#include "esc.h"
#include <stdio.h>
#include <zephyr/kernel.h>

/*********************************** Defines **********************************/
#define MAX_MOTOR_INSTANCES 6

/*********************************** Macros ***********************************/

/*********************************** Typedefs *********************************/
typedef int mix_status_t;

typedef enum {
    MIXER_UAV_CFG_QUADROTOR_X = 0,
    MIXER_UAV_CFG_QUADROTOR_CROSS,
    MIXER_UAV_CFG_HEXAROTOR_X,
    MIXER_UAV_CFG_HEXAROTOR_CROSS
} mixer_uav_cfg_t;

typedef struct {
    uint8_t thrust;
    uint8_t roll;
    uint8_t pitch;
    uint8_t yaw;
} mixer_input_t;

typedef struct {
    esc_t motor_arr[MAX_MOTOR_INSTANCES];
    uint8_t motor_instances;
    mixer_uav_cfg_t uav_config;
} mixer_t;

/********************************** Functions *********************************/
mix_status_t MIXER_AddMotorInstance(esc_t *esc, mixer_t *mixer);

mix_status_t MIXER_Init(mixer_t *mixer, mixer_uav_cfg_t uav_cfg);

mix_status_t MIXER_Execute(mixer_t *mixer, mixer_input_t *mixer_in);

#endif