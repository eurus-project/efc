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
        mixer.motor_instances++;
    } else {
        printk(
            "Error(mixer): Cannor exceed maximum number of motor instances!");
        return -1;
    }

    motor_count++;

    return 0;
}

mix_status_t MIXER_Init(mixer_t *mixer, mixer_uav_cfg_t uav_cfg) {
    /* Add necessary checks here ... */

    mixer->uav_config = uav_cfg;
}

mix_status_t MIXER_Execute(mixer_t *mixer) {}