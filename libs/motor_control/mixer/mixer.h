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

/**
 * @file mixer.h
 * @brief Motor mixing library
 *
 * The mixer transforms normalized control demands (collective thrust and
 * roll/pitch/yaw torque) into per-motor output values. It is a pure
 * input-to-output transformation: it owns no hardware, performs no I/O and
 * keeps no state besides the selected frame geometry. Feeding the resulting
 * motor values to ESCs (and gating them on arming state) is the caller's
 * responsibility.
 *
 * Sign conventions (body frame, viewed from behind / above the craft):
 *  - thrust: collective, 0.0 = none, 1.0 = full
 *  - +roll:  right side down (roll to the right)
 *  - +pitch: nose down (pitch forward)
 *  - +yaw:   nose left (counterclockwise viewed from above)
 */

#ifndef MIXER_H
#define MIXER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIXER_MAX_MOTORS 4

/**
 * Maximal possible magnitude of the torque component.
 */
#define MIXER_TORQUE_HEADROOM 0.3f

/**
 * Pitch and roll to yaw authority ratio.
 */
#define MIXER_RP_TO_YAW_AUTHORITY 10.0f

typedef enum {
    MIXER_SUCCESS = 0,
    MIXER_INVALID_ARG,
    MIXER_INVALID_CFG,
} MIXER_Error_Type;

typedef enum {
    MIXER_FRAME_QUAD_X = 0,
} MIXER_Frame_Type;

/**
 * @brief Motor positions for the MIXER_FRAME_QUAD_X frame
 *
 * These are the indices into MIXER_Output_Type::motor. The caller binds each
 * frame position to the ESC that is physically wired to that motor.
 *
 *              FRONT
 *      [2] CCW      [0] CW
 *           \       /
 *            \     /
 *             (   )
 *            /     \
 *           /       \
 *      [1] CW       [3] CCW
 *              BACK
 */
typedef enum {
    MIXER_QUAD_X_MOTOR_FRONT_RIGHT = 0,
    MIXER_QUAD_X_MOTOR_REAR_LEFT = 1,
    MIXER_QUAD_X_MOTOR_FRONT_LEFT = 2,
    MIXER_QUAD_X_MOTOR_REAR_RIGHT = 3,
} MIXER_Quad_X_Motor_Position_Type;

/**
 * @brief Normalized control demands
 *
 * Required ranges: thrust [0.0, 1.0], roll/pitch/yaw [-1.0, 1.0].
 * MIXER_Calculate's [0, 1] output guarantee is derived assuming these
 * bounds; it does not detect or correct demands that fall outside them, nor
 * does it check for NaN or infinity. Respecting the contract is the
 * caller's responsibility.
 */
typedef struct {
    float thrust;
    float roll;
    float pitch;
    float yaw;
} MIXER_Input_Type;

/**
 * @brief Per-motor output values, each guaranteed to be within [0.0, 1.0]
 *
 * The motor array is indexed by frame position (see
 * MIXER_Quad_X_Motor_Position_Type). Only the first motor_count entries of
 * the frame geometry are written, the rest are left untouched.
 */
typedef struct {
    float motor[MIXER_MAX_MOTORS];
} MIXER_Output_Type;

/**
 * @brief Torque mixing factors of a single motor, one weight per body axis
 */
typedef struct {
    float roll;
    float pitch;
    float yaw;
} MIXER_Torque_Factors_Type;

/**
 * @brief Motor layout of a frame configuration
 *
 * Describes how many motors the frame has and how much torque each of them
 * contributes per axis. The factor array is indexed by frame position and
 * only the first motor_count entries are valid.
 */
typedef struct {
    uint8_t motor_count;
    MIXER_Torque_Factors_Type factor[MIXER_MAX_MOTORS];
} MIXER_Frame_Geometry_Type;

/**
 * @brief Mixer instance, holds the frame geometry selected by MIXER_Init
 *
 * The geometry pointer references a constant table owned by the library and
 * stays NULL until MIXER_Init succeeds.
 */
typedef struct {
    const MIXER_Frame_Geometry_Type *geometry;
    float torque_scale;
} MIXER_Inst_Type;

/**
 * @brief Mixer initialization function
 * @param mixer Pointer to the mixer instance struct
 * @param frame Geometrical frame configuration
 *
 * @retval MIXER_INVALID_ARG mixer is NULL
 * @retval MIXER_INVALID_CFG Unsupported frame configuration
 * @retval MIXER_SUCCESS Mixer initialized successfully
 */
MIXER_Error_Type MIXER_Init(MIXER_Inst_Type *mixer, MIXER_Frame_Type frame);

/**
 * @brief Transforms control demands into per-motor outputs with saturation
 *        protection
 * @param mixer Pointer to the mixer instance struct
 * @param input Pointer to the control demands (not modified)
 * @param output Pointer to the resulting motor outputs
 *
 * Each motor output is composed as:
 *
 *     m_i = T + c_i
 *
 * where T is the collective thrust and c_i is the torque contribution of
 * motor i (the roll/pitch/yaw demands weighted by the frame's mixing
 * factors). Saturation is prevented statically, by construction, rather than
 * detected and corrected at run time:
 *
 * 1. Each axis demand is scaled by torque_scale, computed once by MIXER_Init
 *    as MIXER_TORQUE_HEADROOM divided by the largest per-motor sum of factor
 *    magnitudes. So the worst case (every axis maxed and aligned on one motor)
 *    puts |c_i| at exactly MIXER_TORQUE_HEADROOM, for any demand within the
 *    required range, whatever the per-axis factor magnitudes are. (On quad-X
 *    roll/pitch factors are +-1 and yaw is +-0.1, giving yaw a tenth of the
 *    authority, since it comes from drag torque, not thrust differential.)
 * 2. Thrust is mapped from [0, 1] into [MIXER_TORQUE_HEADROOM,
 *    1 - MIXER_TORQUE_HEADROOM], so the worst-case c_i from step 1 can never
 *    push a motor outside [0, 1] on either end.
 *
 * Together the two steps guarantee 0 <= m_i <= 1 for every motor without
 * this function ever inspecting its own outputs - there is deliberately no
 * run-time saturation check. The cost is fixed and paid on every call, even
 * when the demands are small: collective thrust never reaches 0 or 1, and no
 * single axis ever gets the full [-1, 1] authority to itself. A future
 * dynamic mode can reclaim that range by scaling the combined thrust+torque
 * vector once actual saturation is detected, at the cost of needing that
 * run-time check.
 *
 * @retval MIXER_INVALID_ARG An argument is NULL
 * @retval MIXER_INVALID_CFG Mixer instance holds an unsupported frame
 *                           configuration (e.g. it was never initialized)
 * @retval MIXER_SUCCESS Motor outputs computed successfully
 */
MIXER_Error_Type MIXER_Calculate(const MIXER_Inst_Type *mixer,
                                 const MIXER_Input_Type *input,
                                 MIXER_Output_Type *output);

#ifdef __cplusplus
}
#endif

#endif
