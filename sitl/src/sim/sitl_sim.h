#ifndef _SITL_SIM_H_
#define _SITL_SIM_H_

#include "sitl_def.h"

#include <iostream>

namespace efc
{
namespace sitl
{
    class Simulator
    {
    public:
        Simulator() { std::cout << "Simulator created.\n"; }
        virtual ~Simulator() { { std::cout << "Simulator destroyed.\n"; } }

        static Simulator* CreateSimulator();

    private:

    };
}
}

#endif