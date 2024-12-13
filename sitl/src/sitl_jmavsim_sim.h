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

#include <iostream>

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