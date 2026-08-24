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

#include "mixer.h"

#include <math.h>
#include <stddef.h>

/* Every geometry defined here has to be symmetric in the sense the note on
   MIXER_Calculate describes: each motor carries the collective thrust equally
   (there is no thrust factor, it is implicitly 1.0) and the torque factors of
   each axis sum to zero across the motors. */
static const MIXER_Frame_Geometry_Type quad_x_geometry = {
    .motor_count = 4,
    .factor =
        {
            [MIXER_QUAD_X_MOTOR_FRONT_RIGHT] = {.roll = -1.0f,
                                                .pitch = -1.0f,
                                                .yaw = 0.1f},
            [MIXER_QUAD_X_MOTOR_REAR_LEFT] = {.roll = 1.0f,
                                              .pitch = 1.0f,
                                              .yaw = 0.1f},
            [MIXER_QUAD_X_MOTOR_FRONT_LEFT] = {.roll = 1.0f,
                                               .pitch = -1.0f,
                                               .yaw = -0.1f},
            [MIXER_QUAD_X_MOTOR_REAR_RIGHT] = {.roll = -1.0f,
                                               .pitch = 1.0f,
                                               .yaw = -0.1f},
        },
};

static const MIXER_Frame_Geometry_Type *frame_geometry(MIXER_Frame_Type frame) {
    switch (frame) {
    case MIXER_FRAME_QUAD_X:
        return &quad_x_geometry;
    default:
        return NULL;
    }
}

MIXER_Error_Type MIXER_Init(MIXER_Inst_Type *mixer, MIXER_Frame_Type frame) {
    if (mixer == NULL) {
        return MIXER_INVALID_ARG;
    }

    mixer->geometry = frame_geometry(frame);
    if (mixer->geometry == NULL) {
        return MIXER_INVALID_CFG;
    }

    float max_torque_demand = 0.0f;

    for (size_t i = 0; i < mixer->geometry->motor_count; i++) {
        const float motor_torque_demand =
            fabsf(mixer->geometry->factor[i].roll) +
            fabsf(mixer->geometry->factor[i].pitch) +
            fabsf(mixer->geometry->factor[i].yaw);

        if (motor_torque_demand > max_torque_demand) {
            max_torque_demand = motor_torque_demand;
        }
    }

    mixer->torque_scale = MIXER_TORQUE_HEADROOM / max_torque_demand;

    return MIXER_SUCCESS;
}

MIXER_Error_Type MIXER_Calculate(const MIXER_Inst_Type *mixer,
                                 const MIXER_Input_Type *input,
                                 MIXER_Output_Type *output) {
    if (mixer == NULL || input == NULL || output == NULL) {
        return MIXER_INVALID_ARG;
    }

    /* Protection against non/bad-initialized mixer instance */
    if (mixer->geometry == NULL) {
        return MIXER_INVALID_CFG;
    }

    const float roll = input->roll * mixer->torque_scale;
    const float pitch = input->pitch * mixer->torque_scale;
    const float yaw = input->yaw * mixer->torque_scale;

    /* Compresses thrust into [headroom, 1 - headroom] so the torque budget
       above never pushes a motor outside [0, 1]. */
    const float thrust = MIXER_TORQUE_HEADROOM +
                         input->thrust * (1.0f - 2.0f * MIXER_TORQUE_HEADROOM);

    for (size_t i = 0; i < mixer->geometry->motor_count; i++) {
        const MIXER_Torque_Factors_Type *factor = &mixer->geometry->factor[i];

        output->motor[i] = thrust + roll * factor->roll +
                           pitch * factor->pitch + yaw * factor->yaw;
    }

    return MIXER_SUCCESS;
}
