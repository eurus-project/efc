BUILD:
    To build EFC SITL follow next steps:
        1. mkdir build && cd build
        2. cmake .. -G <your_generator> -DCMAKE_TOOLCHAIN_FILE=../cmake/<your_platform>.cmake -DUSE_GAZEBO=<1 or 0>
            - if GAZEBO = 1 -> Sitl is build for Gazebo Simulator
            - if GAZEBO = 0 -> Sitl is build for JMavSim Simulator (This is default)
        3. <run_your_generator>