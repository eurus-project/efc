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

#if !defined(CONFIG_STATIC_NORM_THRUST_WEIGHT)
#define CONFIG_STATIC_NORM_THRUST_WEIGHT 60
#endif

typedef enum {
    MIXER_SUCCESS = 0,
    MIXER_INIT_ERROR,
    MIXER_ESC_ERROR,
    MIXER_INVALID_CFG,
    MIXER_OUT_OF_BOUND_VALS,
} MIXER_Error_Type;

typedef enum {
    MIXER_UAV_CFG_QUADROTOR_X = 0,
} MIXER_UAV_Cfg_Type;

typedef enum {
    MIXER_NORM_STATIC = 0,
    MIXER_NORM_DYNAMIC,
} MIXER_Normalization_Type;

typedef struct {
    float thrust;
    float roll;
    float pitch;
    float yaw;
} MIXER_Input_Type;

typedef struct {
    float roll_factor;
    float pitch_factor;
    float yaw_factor;
    float throttle_factor;
} MIXER_Motor_Factors_Type;

typedef struct {
    ESC_Inst_Type motor_arr[MAX_MOTOR_INSTANCES];
    MIXER_Motor_Factors_Type motor_factors[MAX_MOTOR_INSTANCES];
    float motor_outputs[MAX_MOTOR_INSTANCES];
    MIXER_UAV_Cfg_Type uav_config;
    MIXER_Normalization_Type norm;
    uint8_t motor_instances;
    uint8_t max_motor_num;
    bool initialized;
} MIXER_Inst_Type;

/**
 * @brief Mixer initialization function
 * @param mixer Pointer to the mixer instance struct
 * @param uav_cfg Geometrical drone configuration
 * @param norm Mixer normalization type, can be:
 *  1. MIXER_NORM_STATIC
 *      Static normalization - conservative worst-case approach.
 *
 *      Quad-X worst case: each axis contributes ±1.0 to motor outputs
 *      Maximum control sum: |roll| + |pitch| + |yaw| = 3.0
 *
 *      Apply constant scale factor to guarantee saturation-free operation
 *      at all thrust levels:
 *        scale = 1.0 / 3.0 = 0.333
 *
 *      Trade-off: Reduces control authority to 33% even when more could
 *      be safely used, but eliminates runtime calculation overhead.
 *
 *  2. MIXER_NORM_DYNAMIC
 *      Dynamic control scaling to prevent motor saturation:
 *
 *      Motor equation: mi = T + ci
 *      where:
 *          mi  - individual motor output [0.0, 1.0]
 *          T   - thrust component (same for all motors)
 *          ci  - control component for motor i (roll, pitch, yaw contribution)
 *
 *      Constraint derivation:
 *          0 <= mi <= 1
 *          0 <= T + ci <= 1
 *          -T <= ci <= 1 - T
 *
 *      Therefore, control magnitude must satisfy:
 *      |ci| <= min(T, 1 - T)
 *
 *      If c_max = |roll| + |pitch| + |yaw| exceeds available headroom, scale
 *      all control inputs proportionally to prevent motor saturation while
 *      preserving thrust authority.
 *
 *      headroom = min(T, 1 - T)
 *
 *      Scale factor: s = headroom / c_max
 *
 * @retval MIXER_INIT_ERROR Error in initialization, wrong args
 * @retval MIXER_INVALID_CFG Invalid or unsupported geometrical configuration
 * @retval MIXER_SUCCESS Mixer initialization successfull
 *
 * @attention MIXER_Init must be called before MIXER_AddMotor call
 */
MIXER_Error_Type MIXER_Init(MIXER_Inst_Type *mixer,
                            const MIXER_UAV_Cfg_Type uav_cfg,
                            const MIXER_Normalization_Type norm);

/**
 * @brief Function to instantiate specific motor
 * @param mixer Pointer to the mixer instance struct
 * @param esc Pointer to the esc instance struct
 *
 * @retval MIXER_INIT_ERROR Error in initialization, wrong args
 * @retval MIXER_ESC_ERROR ESC not initialized correctly
 * @retval MIXER_SUCCESS Mixer initialization succesfull
 *
 * @attention MIXER_AddMotor can be called only after MIXER_Init call
 * @note The motor instances must be added at correct order, based on this
 *       MIXER_UAV_CFG_QUADROTOR_X configuration:
 *
 *         FRONT
 *      [3]     [1]
 *         \   /
 *          [ ]
 *         /   \
 *      [2]     [4]
 *         BACK
 *
 *      Initialization flow should be done like in this code snippet:
 *      MIXER_Init(&mixer, MIXER_UAV_CFG_QUADROTOR_X);
 *
 *      MIXER_AddMotor(&mixer, &esc1);
 *      MIXER_AddMotor(&mixer, &esc2);
 *      MIXER_AddMotor(&mixer, &esc3);
 *      MIXER_AddMotor(&mixer, &esc4);
 *
 *      ,where:
 *      - esc1 represents motor at position [1]
 *      - esc2 represents motor at position [2]
 *      - esc3 represents motor at position [3]
 *      - esc4 represents motor at position [4]
 * @note Currently only quadrotor x geometrical configuration has been supported
 */
MIXER_Error_Type MIXER_AddMotor(MIXER_Inst_Type *mixer, ESC_Inst_Type *esc);

/**
 * @brief Transforms pilot control inputs into motor commands through matrix
 *        mixing, applies saturation protection, and outputs to ESC instances
 * @param mixer Pointer to the mixer instance struct
 * @param mixer_in Pointer to the input data struct. Input format:
 *                 - thrust [0.0, 1.0]
 *                 - roll [-1.0, 1.0]
 *                 - pitch [-1.0, 1.0]
 *                 - yaw [-1.0, 1.0]
 *                 The provided values should be normalized and in specified
 *                 range.
 * @retval MIXER_INIT_ERROR Error in initialization, wrong args or insuficient
 *                          motors has been initialized based on chosen config
 * @retval MIXER_OUT_OF_BOUND_VALS Input values exceed predefined bounds
 * @retval MIXER_ESC_ERROR ESC could not execute motor output set
 * @retval MIXER_SUCCESS Mixer executed successfully
 *
 */
MIXER_Error_Type MIXER_Execute(MIXER_Inst_Type *mixer,
                               MIXER_Input_Type *mixer_in);

#endif
