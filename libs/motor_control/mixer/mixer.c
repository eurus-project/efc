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
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

static void setup_motor_factors(MIXER_Inst_Type *mixer);
static MIXER_Error_Type validate_mixer_input(const MIXER_Input_Type *input);
static void calculate_motor_outputs(MIXER_Inst_Type *mixer,
                                    const MIXER_Input_Type *input);
static MIXER_Error_Type apply_motor_outputs(MIXER_Inst_Type *mixer);
static MIXER_Error_Type validate_mixer_geom_cfg(MIXER_UAV_Cfg_Type cfg);

static void normalize_control(MIXER_Inst_Type *mixer,
                              MIXER_Input_Type *mixer_in);

MIXER_Error_Type MIXER_AddMotor(MIXER_Inst_Type *mixer, ESC_Inst_Type *esc) {
    if (mixer == NULL || esc == NULL) {
        return MIXER_INIT_ERROR;
    }

    if (!mixer->initialized) {
        return MIXER_INIT_ERROR;
    }

    if (!esc->initialized) {
        return MIXER_ESC_ERROR;
    }

    if (mixer->motor_instances == mixer->max_motor_num) {
        return MIXER_INIT_ERROR;
    }

    mixer->motor_arr[mixer->motor_instances] = *esc;
    mixer->motor_instances++;

    return MIXER_SUCCESS;
}

MIXER_Error_Type MIXER_Init(MIXER_Inst_Type *mixer,
                            const MIXER_UAV_Cfg_Type uav_cfg,
                            const MIXER_Normalization_Type norm) {
    MIXER_Error_Type ret = MIXER_INIT_ERROR;

    if (mixer == NULL) {
        return MIXER_INIT_ERROR;
    }

    if (validate_mixer_geom_cfg(uav_cfg) != MIXER_SUCCESS) {
        return MIXER_INVALID_CFG;
    }

    if (norm != MIXER_NORM_STATIC && norm != MIXER_NORM_DYNAMIC) {
        return MIXER_INVALID_CFG;
    }

    memset(mixer->motor_arr, 0, sizeof(mixer->motor_arr));
    memset(mixer->motor_factors, 0, sizeof(mixer->motor_factors));
    memset(mixer->motor_outputs, 0, sizeof(mixer->motor_outputs));
    mixer->motor_instances = 0;
    mixer->max_motor_num = 0;
    mixer->uav_config = uav_cfg;
    mixer->norm = norm;
    mixer->initialized = false;

    switch (uav_cfg) {
    case MIXER_UAV_CFG_QUADROTOR_X:
        mixer->max_motor_num = 4;
        ret = MIXER_SUCCESS;
        break;

    default:
        ret = MIXER_INVALID_CFG;
        break;
    }

    if (ret == MIXER_SUCCESS) {
        mixer->initialized = true;
        setup_motor_factors(mixer);
    } else {
        mixer->initialized = false;
    }

    return ret;
}

MIXER_Error_Type MIXER_Execute(MIXER_Inst_Type *mixer,
                               MIXER_Input_Type *mixer_in) {
    if (mixer == NULL || mixer_in == NULL) {
        return MIXER_INIT_ERROR;
    }

    if (!mixer->initialized) {
        return MIXER_INIT_ERROR;
    }

    // Ensure enough motors have been configured based on geometrical config
    if (mixer->motor_instances != mixer->max_motor_num) {
        return MIXER_INIT_ERROR;
    }

    if (validate_mixer_input(mixer_in) == MIXER_OUT_OF_BOUND_VALS) {
        return MIXER_OUT_OF_BOUND_VALS;
    }

    normalize_control(mixer, mixer_in);
    calculate_motor_outputs(mixer, mixer_in);

    return apply_motor_outputs(mixer);
}

static void setup_motor_factors(MIXER_Inst_Type *mixer) {
    switch (mixer->uav_config) {
    case MIXER_UAV_CFG_QUADROTOR_X:
        // Motor 1 (Front Right - CW): +roll, +pitch, +yaw
        mixer->motor_factors[0].roll_factor = -1.0f;
        mixer->motor_factors[0].pitch_factor = -1.0f;
        mixer->motor_factors[0].yaw_factor = 1.0f;
        mixer->motor_factors[0].throttle_factor = 1.0f;

        // Motor 2 (Rear Left - CW): +roll, +pitch, +yaw
        mixer->motor_factors[1].roll_factor = 1.0f;
        mixer->motor_factors[1].pitch_factor = 1.0f;
        mixer->motor_factors[1].yaw_factor = 1.0f;
        mixer->motor_factors[1].throttle_factor = 1.0f;

        // Motor 3 (Front Left - CCW): +roll, -pitch, -yaw
        mixer->motor_factors[2].roll_factor = 1.0f;
        mixer->motor_factors[2].pitch_factor = -1.0f;
        mixer->motor_factors[2].yaw_factor = -1.0f;
        mixer->motor_factors[2].throttle_factor = 1.0f;

        // Motor 4 (Rear Right - CCW): -roll, +pitch, -yaw
        mixer->motor_factors[3].roll_factor = -1.0f;
        mixer->motor_factors[3].pitch_factor = 1.0f;
        mixer->motor_factors[3].yaw_factor = -1.0f;
        mixer->motor_factors[3].throttle_factor = 1.0f;
        break;

    default:
        break;
    }
}

static MIXER_Error_Type validate_mixer_input(const MIXER_Input_Type *input) {
    if (input == NULL) {
        return MIXER_OUT_OF_BOUND_VALS;
    }

    if (input->thrust < 0.0f || input->thrust > 1.0f || input->roll < -1.0f ||
        input->roll > 1.0f || input->pitch < -1.0f || input->pitch > 1.0f ||
        input->yaw < -1.0f || input->yaw > 1.0f) {
        return MIXER_OUT_OF_BOUND_VALS;
    } else {
        return MIXER_SUCCESS;
    }
}

static void calculate_motor_outputs(MIXER_Inst_Type *mixer,
                                    const MIXER_Input_Type *input) {
    for (uint8_t i = 0; i < mixer->motor_instances; i++) {
        mixer->motor_outputs[i] =
            input->thrust * mixer->motor_factors[i].throttle_factor +
            input->roll * mixer->motor_factors[i].roll_factor +
            input->pitch * mixer->motor_factors[i].pitch_factor +
            input->yaw * mixer->motor_factors[i].yaw_factor;
    }
}

static MIXER_Error_Type apply_motor_outputs(MIXER_Inst_Type *mixer) {
    for (uint8_t i = 0; i < mixer->motor_instances; i++) {
        ESC_Error_Type esc_status =
            ESC_SetSpeed(&mixer->motor_arr[i], mixer->motor_outputs[i]);
        if (esc_status < 0) {
            return MIXER_ESC_ERROR;
        }
    }
    return MIXER_SUCCESS;
}

static void normalize_control(MIXER_Inst_Type *mixer,
                              MIXER_Input_Type *mixer_in) {
    switch (mixer->norm) {
    case MIXER_NORM_STATIC:
        const float thrust_weight = CONFIG_STATIC_NORM_THRUST_WEIGHT / 100.0f;
        const float rpy_weight = 1.0f - thrust_weight;
        mixer_in->thrust *= thrust_weight;
        mixer_in->roll *= rpy_weight;
        mixer_in->pitch *= rpy_weight;
        mixer_in->yaw *= rpy_weight;
        break;

    case MIXER_NORM_DYNAMIC:
        const float c_max = fabsf(mixer_in->roll) + fabsf(mixer_in->pitch) +
                            fabsf(mixer_in->yaw);

        const float headroom = fminf(mixer_in->thrust, 1.0f - mixer_in->thrust);

        if (c_max > headroom && c_max > 0.0f) {
            const float scale = headroom / c_max;

            mixer_in->roll *= scale;
            mixer_in->pitch *= scale;
            mixer_in->yaw *= scale;
        }
        break;

    default:
        break;
    }
}

static MIXER_Error_Type validate_mixer_geom_cfg(MIXER_UAV_Cfg_Type cfg) {
    /* Currently only QUADROTOR_X configuration is implemented.
       When adding support for new geometries, update this validation logic */
    if (cfg == MIXER_UAV_CFG_QUADROTOR_X) {
        return MIXER_SUCCESS;
    }
    return MIXER_INVALID_CFG;
}
