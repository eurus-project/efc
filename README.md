# Eurus flight controller

## Setting up the environment

This project is built as a [workspace](https://docs.zephyrproject.org/4.0.0/develop/application/index.html) Zephyr application, with some additional requirements due to source generation.

1. Install the required dependencies as desribed in the [Zephyr Getting Started#Install Dependencies](https://docs.zephyrproject.org/4.0.0/develop/getting_started/index.html#install-dependencies).
2. Clone the efc repository into an empty directory (e.g. `~/eurus/efc/`). Container directory (`~/eurus/` workspace) will contain Zephyr and additional modules.
>Note: Clone the repository with --recurse-submodules flag to recursively update git submodules.

3. Create a new Python virtual environment in the workspace, install west and configure zephyr-related dependencies.
```bash
cd ~/eurus
python3 -m venv install .venv
source .venv/bin/activate
pip install west
west init -l efc
west update
west zephyr-export
west packages pip --install
west sdk install
deactivate
```
4. Create a efc-specific virtual environment and install the required packages:
```bash
cd ~/eurus/efc
python -m venv python_venv
source python_venv/bin/activate
pip install -r requirements.txt
deactivate
```
> Note: Sourcing the efc virtual environment will not be necessary after the first time as CMake will use the virtual environment python binary.

## Build

Use `west` or CMake to build for one of the supported boards:
```bash
cd ~/eurus
source .venv/bin/activate
cd efc
west build -b eurus_nexus_v1_1 app
```

## EFC SITL
SITL (Software In The Loop) for the EFC autopilot runs the EFC flight control software on a host machine (PC) while interfacing with a flight simulator that models vehicle dynamics and the environment. The simulator provides synthetic sensor data to EFC, and EFC computes and returns actuator commands, forming a closed control loop. This setup enables realistic testing and validation of the EFC autopilot without physical hardware.  
EFC SITL is designed for the EFC autopilot to interact with [jMAVSim](https://github.com/PX4/jMAVSim) simulator.

## Build EFC SITL

### EFC SITL build dependencies

EFC SITL depends on [libuv](https://libuv.org/); therefore, `libuv` must be installed on the host system.  
On Debian-based systems, `libuv` can be installed using:
```bash
apt-get install libuv1-dev
```

### EFC SITL build procedure

1. Create a efc-specific virtual environment and install the required packages (if it hasn't already been created):
```bash
cd ~/eurus/efc
python -m venv python_venv
source python_venv/bin/activate
pip install -r requirements.txt
deactivate
```
> Note: Sourcing the efc virtual environment will not be necessary after the first time as CMake will use the virtual environment python binary.

2. Use CMake to build EFC SITL
```bash
cd ~/eurus/efc
cmake -B build -S sitl
cmake --build build 
```

## Contributing
The `efc` project uses code formatting rules described in `.clang-format`.
To ensure automatic code formatting, use [pre-commit](https://pre-commit.com/) and install hooks:

```bash
pre-commit install
```

The `efc` project utilizes license header checks to ensure the _GPL-3.0_ license is applied to all source files. This process is automated via a GitHub Action using [Hawkeye](https://github.com/korandoru/hawkeye).

To **check locally** for missing license headers, run the following command (using a `podman` or `docker` container):
```bash
podman run -it --rm -v $PWD:/workspace -w /workspace ghcr.io/korandoru/hawkeye:latest check --config /workspace/licenserc.toml
```

To **automatically add** missing license headers, run:
```bash
podman run -it --rm -v $PWD:/workspace -w /workspace ghcr.io/korandoru/hawkeye:latest format --config /workspace/licenserc.toml
```
> Note: The `hawkeye format` command only prepends headers to files that lack them; it does not correct existing, incorrect headers. If a file has an incorrect header, you must manually delete it after the correct one is added.

When creating variables that represent physical units, unit suffixes must be added to their names (e.g. `pulse_duration_us`, `temp_degc`, `gyro_x_radps`).
