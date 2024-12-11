#include <iostream>
#include <memory>

#include "sim/sitl_sim.h"

int main() {
    std::cout << "This is SITL!\n";

    std::unique_ptr<efc::sitl::Simulator>
        Sim(efc::sitl::Simulator::CreateSimulator());

    return 0;
}

