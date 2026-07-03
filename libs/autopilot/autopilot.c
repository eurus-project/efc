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

#include "autopilot/autopilot.h"

#include <math.h>

#define TEST_OUTPUT_MOTOR_COUNT 4
#define TEST_OUTPUT_CENTER 0.15f
#define TEST_OUTPUT_AMPLITUDE 0.15f
#define TEST_OUTPUT_FREQUENCY_HZ 0.2f
#define PI_F 3.14159265358979323846f

static struct imu_6dof_data latest_imu;
static struct baro_data latest_baro;
static struct mag_data latest_mag;
static struct gps_data latest_gps;

void autopilot_init(void) {
    latest_imu = (struct imu_6dof_data){0};
    latest_baro = (struct baro_data){0};
    latest_mag = (struct mag_data){0};
    latest_gps = (struct gps_data){0};
}

void autopilot_imu_input(const struct imu_6dof_data *data) {
    latest_imu = *data;
}

void autopilot_baro_input(const struct baro_data *data) { latest_baro = *data; }

void autopilot_mag_input(const struct mag_data *data) { latest_mag = *data; }

void autopilot_gps_input(const struct gps_data *data) { latest_gps = *data; }

// Skeleton step: no estimator or controller yet, just a slow sine sweep on
// the first four outputs so prop motion is visible in the simulator without
// the vehicle taking off.
void autopilot_step(uint64_t time_us, struct actuator_outputs *out) {
    const float time_s = (float)time_us / 1e6f;
    const float value =
        TEST_OUTPUT_CENTER +
        TEST_OUTPUT_AMPLITUDE *
            sinf(2.0f * PI_F * TEST_OUTPUT_FREQUENCY_HZ * time_s);

    *out = (struct actuator_outputs){0};
    out->timestamp_us = time_us;
    out->armed = true;
    out->count = TEST_OUTPUT_MOTOR_COUNT;
    for (uint8_t i = 0; i < TEST_OUTPUT_MOTOR_COUNT; i++) {
        out->controls[i] = value;
    }
}
