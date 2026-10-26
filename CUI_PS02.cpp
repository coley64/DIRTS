#include "CUI_PS02.h"
#include <Wire.h>

#define CUI_PS02_ADDR 0x6D  // I2C Address of the CUI pressure sensor
#define PS02_REGISTER_PRESSURE 0x06
#define PS02_REGISTER_TEMPERATURE 0x09
#define max_pressure_kPa 35.0


PS02::PS02() {
}

bool PS02::init() {
  this->initialized = true;
  return this->initialized;
}

float PS02::getPressure(void) { // this is gauge, or relative, pressure
  long X0, X1, X2;
  // if (!(this->initialized)) {
  //   return 0x00;
  // }
  float prs;
  Wire.beginTransmission(CUI_PS02_ADDR);
  Wire.write(PS02_REGISTER_PRESSURE);
  Wire.endTransmission();
  Wire.requestFrom(CUI_PS02_ADDR, 3);  // Request three bytes from sensor
  if (Wire.available() <= 3) {         //
    X0 = Wire.read();                  // Reads the data from the register
    X1 = Wire.read();
    X2 = Wire.read();
  }
  long reading = (X0 << 16) + (X1 << 8) + X2;  //read24(PS02_REGISTER_PRESSURE);
  prs = max_pressure_kPa / 0.8f * (reading / 8388608.0f - 0.1f); // pressure in kPa
  prs = 1000.0*prs; // convert from kPa to Pa
  //Serial.printf("PS02 Press: %4.2f\n", prs);
  return prs;
}

float PS02::getTemperature(void) {
  long X0, X1, X2;
  // if (!(this->initialized)) {
  //   return 0x00;
  // }
  float tmp;
  Wire.beginTransmission(CUI_PS02_ADDR);
  Wire.write(PS02_REGISTER_TEMPERATURE);
  Wire.endTransmission();
  Wire.requestFrom(CUI_PS02_ADDR, 3);  // Request three bytes from sensor
  if (Wire.available() <= 3) {         //
    X0 = Wire.read();                  // Reads the data from the register
    X1 = Wire.read();
    X2 = Wire.read();
  }
  long reading = (X0 << 16) + (X1 << 8) + X2;  //read24(PS02_REGISTER_PRESSURE);
  if (reading < 8388608) {
    tmp = reading / 65536.0f + 25.0f;
  } else {
    tmp = (reading - 16777216.0f) / 65536.0f + 25.0f;
  }
  //Serial.printf("PS02 Temp: %3.2f\n", tmp);
  return tmp;
}
