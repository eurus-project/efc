# Eurus flight controller

## Setting up the environment

This project is built as a [workspace](https://docs.zephyrproject.org/4.0.0/develop/application/index.html) Zephyr application, with some additional requirements due to source generation.

1. Install the required dependencies as desribed in the [Zephyr Getting Started#Install Dependencies](https://docs.zephyrproject.org/4.0.0/develop/getting_started/index.html#install-dependencies).
2. Clone the efc repository into an empty directory (e.g. `~/eurus/efc/`). Container directory (`~/eurus/` workspace) will contain Zephyr and additional modules.
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
west build -b blackpill_f411ce app
```


## Contributing

The `efc` project uses code formatting rules described in `.clang-format`.
To ensure automatic code formatting, use [pre-commit](https://pre-commit.com/) and install hooks:

```bash
pre-commit install
```
