// 2026/03/31
// Proof-of-concept for getting healthy motor data

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <driver/i2s.h>

// MPU6050 accelerometer
#define MPU6050_SCL 4
#define MPU6050_SDA 5
#define MPU6050_I2C_ADDR 0x68

// DS18B20 temperature 
#define DS18B20_BUS 12

// Microphone I2S configuration
#define I2S_WS 42  // Word Select / LRCLK
#define I2S_SCK 41 // Serial Clock / BCLK
#define I2S_SD 40  // Serial Data / DIN
#define I2S_PORT I2S_NUM_0

OneWire oneWire(DS18B20_BUS);
DallasTemperature tempSensor(&oneWire);

unsigned long lastTempRequest = 0;
float currentTemp = 0.0;

void setupMPU6050() {
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
}

void setupDS18B20() {
    tempSensor.begin();
    tempSensor.setResolution(12); // standard 12-bit
    tempSensor.setWaitForConversion(false); // non-blocking temperature reading (dont remove)
    tempSensor.requestTemperatures();
    lastTempRequest = millis(); // wait for that 12-bit reading
}

void setupI2S() {
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);
}

void setup() {
  Serial.begin(921600);
  while(!Serial) {
    delay(10);
  }
  setupMPU6050();
  setupDS18B20();
  setupI2S();
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

  // read I2S Audio
  int32_t micSample = 0;
  size_t bytesIn = 0;
  i2s_read(I2S_PORT, &micSample, sizeof(micSample), &bytesIn, portMAX_DELAY);
  
  // shift standard 24/32 bit I2S raw data down for a reasonable amp reading
  int32_t audioAmp = micSample >> 14;

  // accelX, accelY, accelZ, temp, audio
  Serial.print(accelX); Serial.print(",");
  Serial.print(accelY); Serial.print(",");
  Serial.print(accelZ); Serial.print(",");
  Serial.print(currentTemp); Serial.print(",");
  Serial.println(audioAmp); 
}