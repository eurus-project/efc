import serial
import time
import argparse


def main():
    parser = argparse.ArgumentParser(description="Send dummy SBUS data over UART")
    parser.add_argument(
        "--port", 
        type=str, 
        default="/dev/ttyUSB0", 
        help="Serial port to use (e.g., /dev/ttyUSB0, /dev/ttyACM0, COM3)"
    )
    parser.add_argument(
        "--baudrate", 
        type=int, 
        default=100000, 
        help="Baud rate (SBUS uses 100000)"
    )
    parser.add_argument(
        "--interval", 
        type=float, 
        default=0.014, 
        help="Interval between messages in seconds (SBUS sends every ~14ms)"
    )
    
    args = parser.parse_args()
    
    try:
        # Configure serial port
        # SBUS uses 100000 baud, 8E2 (8 data bits, even parity, 2 stop bits)
        ser = serial.Serial(
            port=args.port,
            baudrate=args.baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_EVEN,
            stopbits=serial.STOPBITS_TWO,
            timeout=1
        )
        
        print(f"✓ Connected to {args.port} at {args.baudrate} baud (8E2)")
        print(f"✓ Sending data every {args.interval*1000:.1f}ms")
        print("Press Ctrl+C to stop\n")
        
        message = b"SBUS simulated data\r\n"
        counter = 0
        
        while True:
            ser.write(message)
            counter += 1
            print(f"[{counter:05d}] Sent: {message.decode('ascii', errors='ignore').strip()}")
            time.sleep(args.interval)
            
    except serial.SerialException as e:
        print(f"✗ Serial port error: {e}")
        print(f"\nTip: Check if the device is connected and you have permissions.")
        print(f"     On Linux, you may need to add yourself to the 'dialout' group:")
        print(f"     sudo usermod -a -G dialout $USER")
        return 1
    except KeyboardInterrupt:
        print(f"\n\n✓ Stopped. Sent {counter} messages.")
        ser.close()
        return 0
    except Exception as e:
        print(f"✗ Error: {e}")
        return 1


if __name__ == "__main__":
    exit(main())
