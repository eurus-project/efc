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

#ifndef AUTOPILOT_TYPES_H
#define AUTOPILOT_TYPES_H

#include <stdint.h>

struct imu_6dof_data {
    uint64_t timestamp_us;
    float temperature_degc;
    float accel_mps2[3];
    float gyro_radps[3];
};

struct baro_data {
    uint64_t timestamp_us;
    float temperature_degc;
    float pressure_kpa;
};

struct mag_data {
    uint64_t timestamp_us;
    float mag_gauss[3];
};

struct gps_data {
    uint64_t timestamp_us;
    uint8_t fix_type;
    int32_t lat_dege7;
    int32_t lon_dege7;
    int32_t alt_mm;
    float vel_n_mps;
    float vel_e_mps;
    float vel_d_mps;
    float eph_m;
    float epv_m;
    float cog_deg;
    uint8_t num_sats;
};

#endif // AUTOPILOT_TYPES_H
