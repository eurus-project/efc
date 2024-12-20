# Eurus flight controller

## Building

This project is build as a [freestanding](https://docs.zephyrproject.org/4.0.0/develop/application/index.html) Zephyr app, with some additional requirements due to source generation.

1. Follow the [Zephyr Getting Started](https://docs.zephyrproject.org/4.0.0/develop/getting_started/index.html) instructions for your host OS to obtain the Zephyr repo, submodules and the SDK
2. Create a new Python virtual environment for the project and install the required packages like so:
```bash
python -m venv python_venv
source python_venv/bin/activate
pip install -r requirements.txt
```
> Note: Sourcing the virtual environment will not be necessary after the first time as CMake will use the virtual environment python binary.

3. Source both the Zephyr python virtual environment and the SDK export script
4. Use `west` or CMake to build for one of the supported boards:
```bash
west build -b blackpill_f411ce
```
