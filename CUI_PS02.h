#ifndef CUI_PS02_H
#define CUI_PS02_H

#include "Arduino.h"

class PS02 {
public:
  PS02();
  bool init();
  float getTemperature();
  float getPressure();

protected:
  bool initialized;
  uint8_t slave_address;
  uint32_t read24(uint8_t reg);
  float pressure;     // Pressure value calculated from the raw pressure value
  float temperature;  // Temperature value calculated from the raw temperature value
};

#endif