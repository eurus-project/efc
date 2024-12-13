#include "sitl_sim.h"

#include "sitl_jmavsim_sim.h"

namespace efc
{
namespace sitl
{
    Simulator* Simulator::CreateSimulator()
    {
#if defined(GAZEBO)
        return nullptr;
#else
        return new JMavSimSimulator();
#endif
    }
}
}
