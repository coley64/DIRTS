/*
 * Configuration file for program
 */
#include "Arduino.h"

String FWVERSION = "Rev2.62";  // current firmware version
String FWDATE = "9/29/2025";    // current firmware date
String CNTRLBRD = "RevF";
String SNSRBRD = "None";

//GPRS credentials
const char apn[] = "ne01.vzwstatic";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details to test TCP/SSL
const char server[] = "dev-flood.cema.udel.edu";  // "ares.deos.udel.edu";
const char resource[] = "/lcfs/api/data";         ///flood_sensors/data.php";
const int port = 80;
// API Configuration
#define API_DATA_POST_ENDPOINT_URL "http://dev-flood.cema.udel.edu/lcfs/api/data"  // DOES NOT ACCEPT HTTPS :: HTTP ONLY!
//http://dev-flood.cema.udel.edu/lcfs/web/devices.html

// Over the air (OTA) update configuration - files hosted on the same server as above
const char otaVersionResource[] = "/lcfs/firmware/latest.txt";      // contains "<version> <md5>", ex: "Rev2.63 9f1c2e..."
const char otaFirmwareResource[] = "/lcfs/firmware/firmware.bin";   // Arduino IDE: Sketch > Export Compiled Binary (.ino.bin)
#define OTA_MIN_BATTERY_V 3.7  // do not attempt an update below this battery voltage
#define OTA_TIMEOUT_MS 30000L  // abort download if no data received for this long

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

//I2C Addresses of sensors
#define KELLER_LD_ADDR 0x40  // I2C Address of the Keller pressure/temperature sensor
#define CUI_PS02_ADDR 0x6D   // I2C Address of the CUI pressure sensor
#define LPS33HWADDR_1 0x5C   // I2C Address of the pressure/temperature sensor
#define LPS33HWADDR_2 0x5D   // I2C Address of the pressure/temperature sensor
#define TCAADDR 0x70         // I2C Address of the I2C Mux
#define MS5837ADDR 0x76      // I2C Address of the pressure/temperature sensor
#define MS5803ADDR_1 0x76    // I2C Address of the pressure/temperature sensor
#define MS5803ADDR_2 0x77    // I2C Address of the pressure/temperature sensor
#define BMEADDR 0x77         // I2C Address of the I2C BME280 sensor

//GPIO pin definitions
#define EPD_RESET 2     // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1     // can set to -1 to not use a pin (will wait a fixed delay)
#define SENSOR_RST 2    // GPIO pin connected to reset of sensors and eink
#define MODEM_STATUS 4  // GPIO pin connected to skywire status output (1.8V = ON/ 0 V = OFF)
#define NEOPIXEL 12
#define SNSRBRD_RST 13  // GPIO pin connected to reset_bar (active low) for sensors on sensor board
#define DONE 14
#define EPD_CS 15
#define SW_MODEM_ON_OFF 21  // GPIO pin connected to modem ON/OFF pin, Activate by taking LOW
#define I2C_SCL 22
#define I2C_SDA 23
#define CHARGING 25
#define MODEM_RESET 26
#define EPD_DC 27  // changed to not conflict with SD Card
#define SRAM_CS 32
#define SD_CS_PIN 33  // SPI slave select pin (SS) for SD card (Conflicts with eInk)
#define SNSRBRD_IRQ 34
#define V_BAT 35  // pin connected to voltage divider to measure the battery voltage (input A13 on ADC#2)
#define ETAPE 36  // pin connected to eTape analog output (input A4 on ADC#1)
#define V_IN 39
#define EPD_SPI &SPI  // primary SPI

#define TINY_GSM_MODEM_BG96
#define TINY_GSM_DEBUG SerialMon
#define TINY_GSM_RX_BUFFER 1024  // Set RX buffer to 1Kb or full HTTP response wont be received

#define SerialMon Serial  // Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialAT Serial1  // Set serial for AT commands (to the module)
#define updateMinutes 5   // normal status update time in minutes (default is 5 minutes)
#define sleepTime 60      // light sleep time between indicating status on LEDs in seconds (the longer the less power used)
// #define logDataPeriod 5   //period in minutes to log data to SD CARD
#define ARRAYSIZE 20
#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define COLOR1 EPD_BLACK
#define COLOR2 EPD_RED
#define PressCMD 0x06          // Hexadecimal command to read the pressure
#define TempCMD 0x09           // Hexadecimal command to read the temperature
#define max_pressure_kPa 35.0  // maximum pressure is 35 kPa
#define FluidDensity 999.1616  // density of brackish water at 15C from Kevin Brinson (997 = kg/m^3 for freshwater, 1029 for seawater)
#define I2C_Clock 100000UL     // I2C clock frequency - Accepted values are 100000 (standard mode) and 400000 (fast mode)