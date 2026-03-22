// 2026/03/23
// Proof-of-concept for getting healthy motor data

// INCOMPLETE because the microphone is not yet acquired, 
// making the dataset missing a crucial component

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// MPU6050 accelerometer
#define MPU6050_SCL 4
#define MPU6050_SDA 5

// DS18B20 temperature 
#define DS18B20_BUS 12

Adafruit_MPU6050 mpu;
OneWire oneWire(DS18B20_BUS);
DallasTemperature tempSensor(&oneWire);

unsigned long lastTempRequest = 0;
float currentTemp = 0.0;

void setup() {
  Serial.begin(921600);
  while (!Serial) delay(10);

  // MPU6050 init
  Wire.begin(MPU6050_SDA, MPU6050_SCL);
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_260_HZ);

  // DS18B20 init
  tempSensor.begin();
  tempSensor.setWaitForConversion(false); // non-blocking temperature reading (dont remove)
  tempSensor.requestTemperatures();
  lastTempRequest = millis();
}

void loop() {
  // read MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // read DS18B20 (async)
  if (millis() - lastTempRequest >= 1000) { // check once every 1000ms
    currentTemp = tempSensor.getTempCByIndex(0);
    tempSensor.requestTemperatures();
    lastTempRequest = millis();
  }

  // accelX, accelY, accelZ, temp
  Serial.print(a.acceleration.x); Serial.print(",");
  Serial.print(a.acceleration.y); Serial.print(",");
  Serial.print(a.acceleration.z); Serial.print(",");
  Serial.println(currentTemp); 
}