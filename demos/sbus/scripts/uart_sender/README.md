# SBUS UART Sender

A simple Python script to send dummy SBUS data over UART for testing purposes.

## Setup

1. **Create a virtual environment:**
   ```bash
   python3 -m venv .venv
   ```

2. **Activate the virtual environment:**
   ```bash
   source .venv/bin/activate
   ```

3. **Install dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

## Usage

```bash
python send_serial.py --port /dev/ttyUSB0
```

### Options

- `--port`: Serial port to use (default: `/dev/ttyUSB0`)
  - Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.
  - Windows: `COM3`, `COM4`, etc.
- `--baudrate`: Baud rate (default: `100000` for SBUS)
- `--interval`: Interval between messages in seconds (default: `0.014` = 14ms)

### Examples

```bash
# Use a different port
python send_serial.py --port /dev/ttyACM0

# Slower interval for debugging (1 second)
python send_serial.py --port /dev/ttyUSB0 --interval 1.0
```

Press `Ctrl+C` to stop sending data.

## Troubleshooting

If you get a permission error on Linux, add yourself to the `dialout` group:
```bash
sudo usermod -a -G dialout $USER
```
Then log out and log back in for the changes to take effect.
