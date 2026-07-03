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

#ifndef AUTOPILOT_AUTOPILOT_H
#define AUTOPILOT_AUTOPILOT_H

#include <stdbool.h>
#include <stdint.h>

#include "autopilot/types.h"

// Matches MAVLink HIL_ACTUATOR_CONTROLS' controls[16] field.
#define AUTOPILOT_MAX_ACTUATORS 16

struct actuator_outputs {
    uint64_t timestamp_us;
    float controls[AUTOPILOT_MAX_ACTUATORS];
    uint8_t count;
    bool armed;
};

void autopilot_init(void);

void autopilot_imu_input(const struct imu_6dof_data *data);
void autopilot_baro_input(const struct baro_data *data);
void autopilot_mag_input(const struct mag_data *data);
void autopilot_gps_input(const struct gps_data *data);

// Call once per received sensor sample.
void autopilot_step(uint64_t time_us, struct actuator_outputs *out);

#endif // AUTOPILOT_AUTOPILOT_H
