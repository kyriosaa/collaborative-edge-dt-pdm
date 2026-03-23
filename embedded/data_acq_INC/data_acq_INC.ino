// 2026/03/23
// Proof-of-concept for getting healthy motor data

// INCOMPLETE because the microphone is not yet acquired, 
// making the dataset missing a crucial component

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// MPU6050 accelerometer
#define MPU6050_SCL 4
#define MPU6050_SDA 5
#define MPU6050_I2C_ADDR 0x68

// DS18B20 temperature 
#define DS18B20_BUS 12

OneWire oneWire(DS18B20_BUS);
DallasTemperature tempSensor(&oneWire);

unsigned long lastTempRequest = 0;
float currentTemp = 0.0;

void setup() {
  Serial.begin(921600);
  while(!Serial) {
    delay(10);
  }

  // MPU6050 init 
  // (bare-metal style because I got a clone MPU6050 and the Adafruit library doesn't work)
  Wire.begin(MPU6050_SDA, MPU6050_SCL);
  delay(100);
  Wire.beginTransmission(MPU6050_I2C_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0x00); // wake up at 0
  Wire.endTransmission(true);

  // set accelerometer range to +-16G
  Wire.beginTransmission(MPU6050_I2C_ADDR);
  Wire.write(0x1C); // ACCEL_CONFIG register
  Wire.write(0x18); // 16G (AFS_SEL=3)
  Wire.endTransmission(true);

  // set low pass filter to ~260Hz
  Wire.beginTransmission(MPU6050_I2C_ADDR);
  Wire.write(0x1A); // CONFIG register
  Wire.write(0x00); // DLPF_CFG = 0 (256Hz)
  Wire.endTransmission(true);

  // DS18B20 init
  tempSensor.begin();
  tempSensor.setResolution(12); // standard 12-bit
  tempSensor.setWaitForConversion(false); // non-blocking temperature reading (dont remove)
  tempSensor.requestTemperatures();
  lastTempRequest = millis(); // wait for that 12-bit reading
}

void loop() {
  // read MPU6050
  Wire.beginTransmission(MPU6050_I2C_ADDR);
  Wire.write(0x3B);  // start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_I2C_ADDR, 6, true);  // req 6 reg
  
  // convert to m/s^2 (+-16g = 2048 LSB/g, 1g = 9.80665 m/s^2)
  int16_t rawX = (Wire.read() << 8) | Wire.read();
  int16_t rawY = (Wire.read() << 8) | Wire.read();
  int16_t rawZ = (Wire.read() << 8) | Wire.read();

  float accelX = (rawX / 2048.0) * 9.80665;
  float accelY = (rawY / 2048.0) * 9.80665;
  float accelZ = (rawZ / 2048.0) * 9.80665;

  // read DS18B20 (async)
  if(millis() - lastTempRequest >= 1000) { // check once every 1000ms
    float newTemp = tempSensor.getTempCByIndex(0);
    // getTempCByIndex returns DEVICE_DISCONNECTED_C (-127) on error
    if (newTemp != DEVICE_DISCONNECTED_C) {
        currentTemp = newTemp;
    }
    tempSensor.requestTemperatures();
    lastTempRequest = millis();
  }

  // accelX, accelY, accelZ, temp
  Serial.print(accelX); Serial.print(",");
  Serial.print(accelY); Serial.print(",");
  Serial.print(accelZ); Serial.print(",");
  Serial.println(currentTemp); 
}