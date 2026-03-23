# 2026/03/23
# Proof-of-concept for getting healthy motor data

# INCOMPLETE because the microphone is not yet acquired, 
# making the dataset missing a crucial component

# python -m serial.tools.list_ports

import serial
import csv
import time
import os
from config import COLOR as C

# update this to match the ESP32-S3 Native USB port if changed
SERIAL_PORT = 'COM5' 
BAUD_RATE = 921600 
save_directory = '../datasets/raw/'
csv_filename = 'healthy_motor_baseline_INC.csv'
full_path = os.path.join(save_directory, csv_filename)
os.makedirs(save_directory, exist_ok=True)

# setup port
try:
    ser = serial.Serial()
    ser.port = SERIAL_PORT
    ser.baudrate = BAUD_RATE

    # disabled DTR/RTS otherwise the board will start resetting/halting constantly 
    ser.setDTR(False)
    ser.setRTS(False)
    ser.open()
    
    print(f"Successfully connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
    print("Waiting for sensors to stabilize...")
    time.sleep(2)
    ser.reset_input_buffer() # flush serial buffer
except serial.SerialException:
    print(f"Error: Could not open {SERIAL_PORT}.")
    print("Check the USB connection, port number, and make sure the Arduino IDE Serial Monitor is closed.")
    exit()

# setup csv
with open(full_path, mode='w', newline='') as file:
    writer = csv.writer(file)
    
    # 2-sensor setup bcs I dont have the microphone yet
    writer.writerow(['Accel_X', 'Accel_Y', 'Accel_Z', 'Temperature_C'])
    
    print(f"\nRecording data to {full_path}...")
    print("Ctrl+C to stop recording.")
    
    try:
        start_time = time.time()
        while time.time() - start_time < (3 * 3600): # (3 * 3600) = 3 hours
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                data_points = line.split(',')
                
                # validate that there's the 4 data points before saving
                if len(data_points) == 4:
                    writer.writerow(data_points)
    
                    colored_output = (
                        f"{C['RED']}{data_points[0]:>7}{C['RESET']}, "
                        f"{C['GREEN']}{data_points[1]:>7}{C['RESET']}, "
                        f"{C['BLUE']}{data_points[2]:>7}{C['RESET']}, "
                        f"{C['WHITE']}{data_points[3]:>6}{C['RESET']}"
                    )
                    print(colored_output)
                    
    except KeyboardInterrupt:
        print("\nRecording manually stopped.")
        
print(f"Data successfully saved and closed in {full_path}.")
ser.close()