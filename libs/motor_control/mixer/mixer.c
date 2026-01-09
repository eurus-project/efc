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
static void normalize_motor_outputs(MIXER_Inst_Type *mixer);
static MIXER_Error_Type apply_motor_outputs(MIXER_Inst_Type *mixer);
static MIXER_Error_Type validate_mixer_geom_cfg(MIXER_UAV_Cfg_Type cfg);

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
                            MIXER_UAV_Cfg_Type uav_cfg) {
    MIXER_Error_Type ret = MIXER_INIT_ERROR;

    if (mixer == NULL) {
        return MIXER_INIT_ERROR;
    }

    if (validate_mixer_geom_cfg(uav_cfg) != MIXER_SUCCESS) {
        return MIXER_INVALID_CFG;
    }

    memset(mixer->motor_arr, 0, sizeof(mixer->motor_arr));
    memset(mixer->motor_factors, 0, sizeof(mixer->motor_factors));
    memset(mixer->motor_outputs, 0, sizeof(mixer->motor_outputs));
    mixer->motor_instances = 0;
    mixer->max_motor_num = 0;
    mixer->uav_config = uav_cfg;
    mixer->initialized = false;

    switch (uav_cfg) {
    case MIXER_UAV_CFG_QUADROTOR_X:
    case MIXER_UAV_CFG_QUADROTOR_CROSS:
        mixer->max_motor_num = 4;
        ret = MIXER_SUCCESS;
        break;
    case MIXER_UAV_CFG_HEXAROTOR_X:
    case MIXER_UAV_CFG_HEXAROTOR_CROSS:
        mixer->max_motor_num = 6;
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
                               const MIXER_Input_Type *mixer_in) {
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

    calculate_motor_outputs(mixer, mixer_in);

    normalize_motor_outputs(mixer);

    return apply_motor_outputs(mixer);
}

static void setup_motor_factors(MIXER_Inst_Type *mixer) {
    switch (mixer->uav_config) {
    case MIXER_UAV_CFG_QUADROTOR_X:
        // Motor 1 (Front Right): +roll, +pitch, +yaw
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

static void normalize_motor_outputs(MIXER_Inst_Type *mixer) {
    if (mixer->motor_instances == 0) {
        return;
    }

    // Find min and max outputs
    float max_output = mixer->motor_outputs[0];
    float min_output = mixer->motor_outputs[0];

    for (uint8_t i = 1; i < mixer->motor_instances; i++) {
        if (mixer->motor_outputs[i] > max_output) {
            max_output = mixer->motor_outputs[i];
        }
        if (mixer->motor_outputs[i] < min_output) {
            min_output = mixer->motor_outputs[i];
        }
    }

    // If max exceeds 1.0, scale all down proportionally
    if (max_output > 1.0f) {
        float scale = 1.0f / max_output;
        for (uint8_t i = 0; i < mixer->motor_instances; i++) {
            mixer->motor_outputs[i] *= scale;
        }
        // Recalculate min after scaling
        min_output *= scale;
    }

    // If min below 0.0, shift all up
    if (min_output < 0.0f) {
        for (uint8_t i = 0; i < mixer->motor_instances; i++) {
            mixer->motor_outputs[i] -= min_output;
        }
    }

    // Final safety clamp to [0.0, 1.0]
    for (uint8_t i = 0; i < mixer->motor_instances; i++) {
        if (mixer->motor_outputs[i] < 0.0f) {
            mixer->motor_outputs[i] = 0.0f;
        }
        if (mixer->motor_outputs[i] > 1.0f) {
            mixer->motor_outputs[i] = 1.0f;
        }
    }
}

static MIXER_Error_Type validate_mixer_geom_cfg(MIXER_UAV_Cfg_Type cfg) {
    // Validate that the config is within the valid enum range
    if (cfg >= MIXER_UAV_CFG_QUADROTOR_X &&
        cfg <= MIXER_UAV_CFG_HEXAROTOR_CROSS) {
        return MIXER_SUCCESS;
    }
    return MIXER_INVALID_CFG;
}