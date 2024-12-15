/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024 The efc developers.
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

#ifndef _SITL_JMAVSIM_SIM_H_
#define _SITL_JMAVSIM_SIM_H_

#include "sitl_sim.h"

// TODO: Include mavlink headers for:
// mavlink_hil_sensor_t - RECV
// mavlink_hil_optical_flow_t - RECV
// mavlink_odometry_t - RECV
// mavlink_vision_position_estimate_t - RECV
// mavlink_distance_sensor_t - RECV
// mavlink_hil_gps_t - RECV
// mavlink_rc_channels_t - RECV
// mavlink_landing_target_t - RECV
// handle_message_hil_state_quaternion - RECV
// mavlink_raw_rpm_t - RECV
// mavlink_hil_actuator_controls_t - SEND

namespace efc
{
namespace sitl
{
    class JMavSimSimulator : public Simulator
    {
    public:
        JMavSimSimulator() { std::cout << "JMavSim created.\n"; }
        ~JMavSimSimulator() { std::cout << "JMavSim destroyed.\n"; }
        
    private:

    };
}
}

#endif