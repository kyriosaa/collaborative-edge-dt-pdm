# 2026/03/23
# Proof-of-concept for getting healthy motor data

# INCOMPLETE because the microphone is not yet acquired, 
# making the dataset missing a crucial component

# python -m serial.tools.list_ports

import serial
import csv
import time
import os

# update this to match the ESP32-S3 Native USB port if changed
SERIAL_PORT = 'COM5' 
BAUD_RATE = 921600 
save_directory = '../datasets/raw/'
csv_filename = 'healthy_motor_baseline_INC.csv'
full_path = os.path.join(save_directory, csv_filename)
os.makedirs(save_directory, exist_ok=True)

# setup port
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
    print(f"Successfully connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
except serial.SerialException:
    print(f"Error: Could not open {SERIAL_PORT}.")
    print("Check the USB connection, port number, and make sure the Arduino IDE Serial Monitor is closed.")
    exit()

# setup csv
with open(full_path, mode='w', newline='') as file:
    writer = csv.writer(file)
    
    # 2-sensor setup bcs I dont have the microphone yet
    writer.writerow(['Accel_X', 'Accel_Y', 'Accel_Z', 'Temperature_C'])
    
    print(f"\nRecording Phase 01 Data to {full_path}...")
    print("Ctrl+C to stop recording.")
    
    try:
        start_time = time.time()
        # Defaulted to a 10-second test run. 
        # Change the '10' below to '(3 * 3600)' when you do the real 3-hour run.
        while time.time() - start_time < 10: 
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                data_points = line.split(',')
                
                # validate that there's the 4 data points before saving
                if len(data_points) == 4:
                    writer.writerow(data_points)
                    
    except KeyboardInterrupt:
        print("\nRecording manually stopped by user.")
        
print(f"Data successfully saved and closed in {full_path}.")
ser.close()