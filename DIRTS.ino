/***************************************************************************************
      DEOS_DELDOT_QBG96
      Description: Program reads sensors and sends data to web server. It als stores
              data on SD Card and displays status on LCD.
      Current sensors:
            GPS/time - time/latitude/longitude taken from Nimbelink QBG96 Skywire module
            BME280 - temp/pressure/humidity
            battery level/charge status
      Data is logged every 5 minutes.
      RevB: ESP32 based, added TinyGSM
      RevD: add eInk/ePaper display, printf
      RevD1: add low power operation/sleep
      RevD7: status updates every 5 minutes new sensor board rev5B
      RevD9: use open drain gpio for modem reset and on/off
      RevE1: recognize new sensor boards and sensors
      Revf1: added watchdog reset, f2 15 min timeout, f3 store old lat/lon/offsets
      revB2.0 : added Keller pressure sensors and removed old remote sensors
                changed reference pressure to BME280 instead of MS5837
      revC1.0: updated for new e-ink and removed ESP32AnalogRead
      revF1.0: use JSON data format - sned data to dev-flood.cema.udel.edu
                   written by Rick Martin 3/24/2021
**************************************************************************************/
#include "config.h"  //local configuration information
#include "Arduino.h"
#include "esp_system.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SD.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MS5837.h>
#include <KellerLD.h>
#include "CUI_PS02.h"
#include <Adafruit_ThinkInk.h>
#include <Adafruit_EPD.h>

Adafruit_NeoPixel pixels(2, NEOPIXEL, NEO_GRB + NEO_KHZ800);
MS5837 controlbrd;
Adafruit_BME280 bme;  // I2C
KellerLD keller;
PS02 ps02;
ThinkInk_213_Mono_BN display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, EPD_SPI);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);
File dataFile, statusFile;
float SD_MB_Total = 0.0;
float SD_MB_Free = 0.0;
float SD_MB_Used = 0.0;
Preferences eepromData;

boolean startup = true;          //if true restart the modem, otherwise initializze the modem
boolean GPSstartup = true;       //if true use multiple attempts to get GPS location
boolean getNetworkTime = true;   //if true get local network time from cellular nework
boolean getLocation = true;      //if true get GPS location
boolean updateReadings = false;  //if true display/store/send data
boolean updateStatus = false;    //if true post new status settings to cloud
boolean updateDataLog = false;
boolean POST_DATA = false;  //if true send current status and data over cellular link to web server
boolean flooded = false;    //if true the water level sensors indicate flooding
boolean BME280 = false;
boolean MS5837 = false;
boolean sensorKeller = false;
boolean sensorCUI = false;
boolean connectNetwork;
boolean GPSfix = false;
boolean GPSLOC = false;
boolean SKYWIRE = false; 
boolean NetworkConnected = false;
boolean validDate = false;  // true if able to get network time
boolean validFile = false;
boolean LogDataSDCard = true;  //if true write data to SD card, don't if false
boolean done, charging;
int resetCounter;
int R, G, B;
float waterlevel = 0;
int lastMinute, lastDay;
int GPSsatellites;
int SIGSTRENGTH;
int MODEMTEMP;
int yr, mnth, dy, hr, mins, secs;  //, GPSweekday
int SleepSeconds;
unsigned status;             //BME280 status
unsigned long readings = 0;  //Number of data points taken
float reference_pressure;
float BMEtempC, BMEpressure, BMEhumidity;  //environmental data form control baor BME280 sensor
float MS5837_0TempC, MS5837_Pressure;      //data from controlboard MS5837 pressure sensor
float kellerPressure, kellerTempC, kellerOffset, kellerDepth;
float CUIPressure, CUITempC, CUIOffset, CUIDepth;
double inputV;
int timezone = 0;
float GPSlatitude, GPSlongitude, GPSaltitidue, GPSaccuracy;
String GPSDate = "NotAquired";  // last date and time of successfull GPS location
String GPSTime = "NotAquired";  // last date and time of successfull GPS location
double batteryVoltage;
char dataFileChar[23], statusFileChar[25];
String modemResp;
String ESP32MACADDRESS = "";  // ESP32 MAC Address
String DEVICE_ID = "";        // IMEI for Skywire module [(String)"862785049468864"]
String SIMCCID = "";          // SIM card CCID number
String MODEMTYPE = "";        // Cellular Modem revision
String MODEMTECH = "";        // Cellular Modem technology (CATM1)
String jsonFileName;
String DateString = "";
String TimeString = "";
String NetDateString = "";
String NetTimeString = "";
String LocationString = "";
String jsondata;


void setup() {
  setCpuFrequencyMhz(80);
  SerialMon.begin(115200);
  pinMode(CHARGING, INPUT);
  pinMode(DONE, INPUT);
  pinMode(SNSRBRD_IRQ, INPUT);
  pinMode(MODEM_STATUS, INPUT);
  pinMode(SNSRBRD_RST, OUTPUT);
  pinMode(SENSOR_RST, OUTPUT);
  pinMode(SW_MODEM_ON_OFF, GPIO_MODE_OUTPUT_OD);  // Make the modem ON/OFF pin be an open drain pin (w/internal pullup to 1.8V)
  pinMode(MODEM_RESET, GPIO_MODE_OUTPUT_OD);      // Make the modem reset pin be an open drain pin (w/internal pullup to 1.8V)
  eepromData.begin("my-data", false);             //setup namespace for nonvolatile R/W mode data storage
  delay(2000);
  SerialMon.println("\n\nUniversity of Delaware DELDOT Flood Sensor");
  SerialMon.println("Software version:" + FWVERSION);
  SerialMon.println("Firmware Date:" + FWDATE);
  SerialMon.println("Control Board:" + CNTRLBRD);
  pixels.begin();                        // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();                        // Set all pixel colors to 'off'
  strobeLED(0, 25, 25, 25, 100, 25, 4);  //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
  strobeLED(1, 25, 25, 25, 100, 25, 4);  //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
  display.begin(THINKINK_MONO);
  display.clearBuffer();
  display.setCursor(0, 0);
  display.setTextColor(COLOR1);
  display.setTextWrap(false);
  display.println("UD/DelDOT LCFS STARTUP.");
  display.println("Software:" + FWVERSION);
  display.println("FW Date:" + FWDATE);
  display.printf("Update:%d min.\n", updateMinutes);
  //eepromData.clear();  //uncomment to reset all stored values

  resetCounter = eepromData.getInt("counter", 0);             // get nonvolatile count of number of resets
  resetCounter++;                                             // increment the reset counter
  eepromData.putInt("counter", resetCounter);                 // Store the counter to the Preferences
  GPSlatitude = eepromData.getFloat("lat", 39.6803354);       // DEOS lab location 39.68033541932235, -75.74836244196217
  GPSlongitude = eepromData.getFloat("lon", -75.7483624);     //
  eepromData.end();                                           // Close the nonvolatile namesake
  display.printf("RESET counter value: %d\n", resetCounter);  // Display the reset counter to Eink
  display.display();
  delay(2000);
  display.clearBuffer();
  display.setCursor(0, 0);
  display.setTextColor(COLOR1);
  display.setTextWrap(false);
  display.println("UD/DelDOT SENSORS.");
  WiFi.mode(WIFI_STA);  // Set Wi-Fi mode to station mode
  WiFi.begin();         // Initialize the Wi-Fi interface
  ESP32MACADDRESS = WiFi.macAddress();
  WiFi.mode(WIFI_OFF);  // Sets the WiFi mode to off
  Wire.begin(I2C_SDA, I2C_SCL, I2C_Clock);
  SerialMon.println("ESP32 MAC address:" + ESP32MACADDRESS);
  SerialMon.printf("Standard update time is %d min.\n", updateMinutes);
  SerialMon.printf("RESET counter value: %d\n", resetCounter);  // Display the reset counter to Serial Monitor
  SerialMon.println("UD/DELDOT LCFS. SENSOR STATUS:");
  digitalWrite(MODEM_RESET, HIGH);
  digitalWrite(SW_MODEM_ON_OFF, HIGH);
  if (digitalRead(MODEM_STATUS)) {  // Skywire modem is on so turn it of
    SerialMon.println("Turning off Skywire modem.");
    display.println("Turning off Skywire modem.");
    digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn off Skywire modem
    delay(2000);                         // modem requires >650ms pulse to turn off
    digitalWrite(SW_MODEM_ON_OFF, HIGH);
  }

  int NumSensors = InitSensors();
  SerialMon.print("Sensor Board:" + SNSRBRD);
  SerialMon.printf(", %d sensors found.\n\n", NumSensors);
  display.print("Sensor Brd:" + SNSRBRD);
  display.printf(", %d sensors found.\n\n", NumSensors);
  analogReadResolution(12);
  batteryVoltage = 0.002 * analogReadMilliVolts(V_BAT);  //readBatVoltage();
  inputV = 0.0025 * analogReadMilliVolts(V_IN);          //readSolar or power supply input voltage;
  done = !digitalRead(DONE);                             //invert so 1 is true
  charging = !digitalRead(CHARGING);
  display.printf("bat:%4.2fV,solar input%4.2fV\n", batteryVoltage, inputV);
  SerialMon.printf("\tbattery %4.2fV, inputV %4.2fV, CHARGING: %d, DONE: %d\n", batteryVoltage, inputV, charging, done);

  if (LogDataSDCard) {
    if (!SD.begin(SD_CS_PIN)) {
      SerialMon.println("SD Card initialization failed!");
      display.println("No SD card found!");
      LogDataSDCard = false;
    } else {
      SerialMon.println("Initializing SD card:");
      display.println("SD card initialized!");
      //SerialMon.printf("\tLog data to SD Card every %d minutes.\n", logDataPeriod);
      //display.printf("Log data to SD Card %d min.\n", logDataPeriod);
      uint8_t cardType = SD.cardType();
      if (cardType == CARD_NONE) {
        SerialMon.println("\tSD card attached");
        display.println("No SD card!");
        LogDataSDCard = false;
      }
      SerialMon.print("\tSD Card Type: ");
      display.print("SD Card Type: ");
      if (cardType == CARD_MMC) {
        SerialMon.println("MMC");
        display.println("MMC");
      } else if (cardType == CARD_SD) {
        SerialMon.println("SDSC");
        display.println("SDSC");
      } else if (cardType == CARD_SDHC) {
        SerialMon.println("SDHC");
        display.println("SDHC");
      } else {
        SerialMon.println("UNKNOWN");
        display.println("UNKNOWN");
      }
      SD_MB_Total = SD.cardSize() / (1024.0 * 1024.0);
      SerialMon.printf("\tSD Card Size: %6.1fMB\n", SD_MB_Total);
      display.printf("SD Card Size: %6.1fMB\n", SD_MB_Total);
    }
  }
  display.println("Sensor Setup complete.");
  SerialMon.println("Sensor Setup complete.");
  display.display();
  delay(2000);
  display.clearBuffer();
  display.setCursor(0, 0);
  display.setTextColor(COLOR1);
  display.setTextWrap(false);
  SerialMon.println("\nUD/DELDOT LCFS. SKYWIRE STATUS:");
  display.println("UD/DELDOT LCFS. SKYWIRE STATUS:");
  SKYWIRE = Skywire();  //Initialize Nimbelink Skywire cellular modem, returns true if able to connect to cellular network
  if (SKYWIRE) {
    startup = true;
    TimeString = "";
    DateString = String(yr) + "/";
    if (mnth < 10) DateString = DateString + "0";
    DateString = DateString + String(mnth) + "/";
    if (dy < 10) DateString = DateString + "0";
    DateString = DateString + String(dy);
    if (hr < 10) TimeString = "0";
    TimeString = TimeString + String(hr) + ":";
    if (mins < 10) TimeString = TimeString + "0";
    TimeString = TimeString + String(mins) + ":";
    if (secs < 10) TimeString = TimeString + "0";
    TimeString = TimeString + String(secs);
    validDate = true;
    getNetworkTime = false;
  } else {
    startup = true;
    strobeLED(2, 100, 0, 0, 50, 50, 8);  //flash red LEDs to indicate startup failure ... restart?
    SerialMon.println("Modem Initialized failed.");
    display.println("Modem Init failed.");
    yr = 2025;
    mnth = 1;
    dy = 1;
    hr = 12;
    mins = 34;
    secs = 56;
    setTime(hr, mins, secs, dy, mnth, yr);  //create fake date and time if not able to get network time
    DateString = String(yr) + "/";
    if (mnth < 10) DateString = DateString + "0";
    DateString = DateString + String(mnth) + "/";
    if (dy < 10) DateString = DateString + "0";
    DateString = DateString + String(dy);
    TimeString = "";
    if (hr < 10) TimeString = "0";
    TimeString = TimeString + String(hr) + ":";
    if (mins < 10) TimeString = TimeString + "0";
    TimeString = TimeString + String(mins) + ":";
    if (secs < 10) TimeString = TimeString + "0";
    TimeString = TimeString + String(secs);
    SerialMon.print("Simulated Local Network Time: ");
    SerialMon.println(TimeString + "; " + DateString);
    display.println("Simulated Local Network Time: ");
    display.println(TimeString + "; " + DateString);
    validDate = false;
    getNetworkTime = true;
  }
  if (LogDataSDCard && validDate) {
    createDataFile();
    validFile = true;
    SerialMon.println("Created:" + jsonFileName);
    display.println("Created:" + jsonFileName);
  } else {
    SerialMon.println("No file created.");
    display.println("No file created.");
    validFile = false;
  }
  SerialMon.println("End setup *******************\n");
  display.println("End setup********************\n");
  display.display();
  delay(2000);
  updateReadings = true;
  updateStatus = true;
  updateDataLog = false;
  for (int i = 0; i < 5; i++) {
    strobeLED(0, 0, 0, 50, 50, 150, 1);  //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
    strobeLED(1, 0, 0, 50, 50, 150, 1);  //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
  }
  /* WATCHDOG TIMER */
  //   //timer = timerBegin(0, 80, true);               //timer 0, div 80
  // timer = timerBegin(1000000);               //hw_timer_t * timerBegin(uint32_t frequency);
  // //timerAttachInterrupt(timer, &watchdog, true);  //attach callback
  //   timerAttachInterrupt(timer, &watchdog);  //attach callback
  // timerAlarmWrite(timer, 900000000ULL, false);   //set time in seconds  [15 minutes]
  // timerAlarmEnable(timer);                       //enable interrupt
}

void loop() {
  //timerWrite(timer, 0);                          //reset timer (feed watchdog)
  hr = hour();
  mins = minute();
  secs = second();
  yr = year();
  mnth = month();
  dy = day();
  DateString = String(yr) + "/";
  if (mnth < 10) DateString = DateString + "0";
  DateString = DateString + String(mnth) + "/";
  if (dy < 10) DateString = DateString + "0";
  DateString = DateString + String(dy);
  TimeString = "";
  if (hr < 10) TimeString = "0";
  TimeString = TimeString + String(hr) + ":";
  if (mins < 10) TimeString = TimeString + "0";
  TimeString = TimeString + String(mins) + ":";
  if (secs < 10) TimeString = TimeString + "0";
  TimeString = TimeString + String(secs);
  if ((hour() == 12) && (minute() == 0)) {  //midday start new datafile and get time and location
    getNetworkTime = true;
    getLocation = true;
    GPSstartup = true;
    readings = 0;
    if (LogDataSDCard) {
      dataFile.close();
      statusFile.close();
      SD_MB_Total = SD.totalBytes() / (1024.0 * 1024.0);
      SD_MB_Used = SD.usedBytes() / (1024.0 * 1024.0);
      SD_MB_Free = SD_MB_Total - SD_MB_Used;
      SerialMon.printf("SD Card Total space: %6.1fMB\n", SD_MB_Total);
      SerialMon.printf("SD Card Used space: %6.1fMB\n", SD_MB_Used);
      SerialMon.printf("SD Card Fres space: %6.1fMB\n\n\n", SD_MB_Free);
      createDataFile();
    }
  }
  batteryVoltage = 0.002 * analogReadMilliVolts(V_BAT);  //readBatVoltage();
  inputV = 0.0025 * analogReadMilliVolts(V_IN);          //readSolar or power supply input voltage;
  done = !digitalRead(DONE);                             //invert so 1 is true
  charging = !digitalRead(CHARGING);
  if (batteryVoltage > 3.7) {  // GREEN Led indicator
    R = 0;
    G = 50;
    B = 0;
  } else if (batteryVoltage < 3.5) {  // RED Led indicator
    R = 50;
    G = 0;
    B = 0;
  } else {  // YELLOW Led indicator
    R = 25;
    G = 25;
    B = 0;
  }
  strobeLED(0, R, G, B, 50, 150, 2);      //LED0 = battery level
  if (done) {                             //Battery is fully charged - LED1 = Green
    strobeLED(1, 0, 50, 0, 50, 150, 2);   //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
  } else if (charging) {                  //Battery is charging - LED1 = blue
    strobeLED(1, 0, 0, 50, 50, 150, 2);   //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
  } else {                                //Battery is not charging - LED1 = red
    strobeLED(1, 50, 00, 0, 50, 150, 2);  //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
  }

  if (minute() == int(minute() / updateMinutes) * updateMinutes) {  // time to send new data and uodate SD file if logging
    flooded = false;
    waterlevel = 0;
    DynamicJsonDocument doc(1024);
    doc["deviceId"] = ESP32MACADDRESS;
    doc["timestamp"] = DateString + "," + TimeString;
    JsonObject device = doc.createNestedObject("device");
    device["controllerFirmwareRevision"] = FWVERSION;
    device["FirmwareDate"] = FWDATE;
    device["controllerHardwareRevision"] = CNTRLBRD;
    device["macAddress"] = ESP32MACADDRESS;
    device["modemImei"] = DEVICE_ID;
    device["modemTech"] = MODEMTECH;
    device["modemVersion"] = MODEMTYPE;
    device["SIM"] = SIMCCID;
    JsonObject status = doc.createNestedObject("status");
    status["BME280"] = BME280;
    status["MS5837"] = MS5837;
    status["PCA9548A"] = true;
    status["KellerLD"] = sensorKeller;
    status["PS02"] = sensorCUI;
    status["solarVoltage"] = inputV;
    status["batteryVoltage"] = batteryVoltage;
    status["batteryCharging"] = charging;
    status["SdInstalled"] = LogDataSDCard;
    status["SdSpaceFree"] = SD_MB_Free;
    status["SdSpaceUsed"] = SD_MB_Used;
    status["cellQuality"] = SIGSTRENGTH;
    status["cellTemperature"] = MODEMTEMP;
    status["resetCounter"] = resetCounter;
    JsonObject location = doc.createNestedObject("location");
    location["longitude"] = GPSlongitude;
    location["latitude"] = GPSlatitude;
    location["time"] = GPSTime;
    location["date"] = GPSDate;
    JsonObject data = doc.createNestedObject("data");
    TCA9548A(0);
    delay(200);
    if (MS5837) {  // get reference atmospheric pressure in box off of MS5837 sensor
      controlbrd.read();
      MS5837_0TempC = controlbrd.temperature();
      MS5837_Pressure = controlbrd.pressure() * 100;  // pressure in Pa (1mbar - 100 Pa)
      reference_pressure = BMEpressure;
      data["MS5837_Pressure"] = MS5837_Pressure;
      data["MS5837_Temperature"] = MS5837_0TempC;
    } else {
      reference_pressure = 0.0;
      data["MS5837_Pressure"] = -999;
      data["MS5837_Temperature"] = -999;
    }
    if (BME280) {
      BMEtempC = bme.readTemperature();
      BMEpressure = bme.readPressure();
      BMEhumidity = bme.readHumidity();
      data["BME280_Humidity"] = BMEhumidity;
      data["BME280_Pressure"] = BMEpressure;
      data["BME280_Temperature"] = BMEtempC;
    } else {
      data["BME280_Humidity"] = -999;
      data["BME280_Pressure"] = -999;
      data["BME280_Temperature"] = -999;
    }
    TCA9548A(1);
    delay(200);
    if (sensorKeller) {
      keller.read();
      kellerTempC = keller.temperature();
      kellerPressure = keller.pressure() * 100;  // pressure in Pa (1mbar - 100 Pa)
      data["KellerLD_Pressure"] = kellerPressure;
      data["KellerLD_Temperature"] = kellerTempC;
      kellerDepth = ((kellerPressure - reference_pressure + kellerOffset) / (FluidDensity * 9.80665)) * 3937;
      if (kellerDepth > 2) flooded = true;
      waterlevel = kellerDepth;
    } else {
      data["KellerLD_Pressure"] = -999;
      data["KellerLD_Temperature"] = -999;
    }
    if (sensorCUI) {
      CUIPressure = ps02.getPressure();
      CUITempC = ps02.getTemperature();
      data["PS02_Pressure"] = CUIPressure;
      data["PS02_Temperature"] = CUITempC;
      CUIDepth = ((CUIPressure + CUIOffset) / (FluidDensity * 9.80665)) * 3937;
      if (CUIDepth > 2) flooded = true;
      waterlevel = CUIDepth;
    } else {
      data["PS02_Pressure"] = -999;
      data["PS02_Temperature"] = -999;
    }
    serializeJsonPretty(doc, Serial);
    if (LogDataSDCard && validFile) {  // write JSON to data file
      serializeJson(doc, dataFile);
      SD_MB_Total = SD.totalBytes() / (1024.0 * 1024.0);
      SD_MB_Used = SD.usedBytes() / (1024.0 * 1024.0);
      SD_MB_Free = SD_MB_Total - SD_MB_Used;
    }
    //***************
    // send data to webserver
    serializeJson(doc, jsondata);
    POST_DATA = true;
    if (!Skywire()) {                       // did not POST data correctly
      startup = true;                       //
      strobeLED(2, 50, 0, 0, 250, 250, 2);  //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
    } else {                                // successfully POSTed JSON data
      strobeLED(2, 0, 0, 50, 250, 250, 2);  //strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
    }
    displayValues();
    SleepSeconds = 60 - second();
    SerialMon.printf("\nFirst light Sleep for %d seconds. \n", SleepSeconds);
    esp_sleep_enable_timer_wakeup(SleepSeconds * uS_TO_S_FACTOR);  // light sleep
    esp_light_sleep_start();
  } else {  // Not posting new data so sleep until next datareading/logging/posting
    SleepSeconds = 60 - second();
    SerialMon.printf("Light Sleep for %d seconds. \n", SleepSeconds);
    SerialMon.flush();
    esp_sleep_enable_timer_wakeup(SleepSeconds * uS_TO_S_FACTOR);  // light sleep until next reading time
    esp_light_sleep_start();
  }
}

int InitSensors() {  //scan I2C bus for devices, assume TCA9548 I2C mux at 0x70
  byte error, address, mux;
  int nDevices;

  nDevices = 0;
  digitalWrite(SNSRBRD_RST, LOW);
  digitalWrite(SENSOR_RST, LOW);
  delay(1000);
  digitalWrite(SNSRBRD_RST, HIGH);
  digitalWrite(SENSOR_RST, HIGH);
  SerialMon.println("Scanning I2C for sensors...");
  TCA9548A(0);
  delay(200);
  SerialMon.println("Scanning I2C bus with TCA9548 I2C mux set to 0.");
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      SerialMon.print("I2C device found at address 0x");
      if (address < 16) {
        SerialMon.print("0");
      }
      SerialMon.println(address, HEX);
      switch (address) {
        case TCAADDR:
          SerialMon.println("\t\tTCA9548 I2C Mux.");
          break;
        case MS5837ADDR:  // I2C Address of the MS5837 (0x76) pressure/temperature sensor on sensor board1
          nDevices++;
          SerialMon.println("\tControl Board MS5837.");
          if (controlbrd.init()) {  //sensorbrd, controlbrd;
            MS5837 = true;
            R = 0;
            G = 50;
            B = 0;
            display.println("MS5837 initilized.");
            SerialMon.println("\t\tMS5837 initilized.");
            controlbrd.setModel(MS5837::MS5837_02BA);  //sensorbrd, controlbrd;
            controlbrd.setFluidDensity(997);           // kg/m^3 (freshwater, 1029 for seawater)
            controlbrd.read();
            MS5837_Pressure = controlbrd.pressure() * 100;  // pressure in Pa (1mbar - 100 Pa)
            display.printf("Ref: %6.1f Pa, \n", MS5837_Pressure);
            SerialMon.printf("\t\tMS5837 pressure: %6.1f Pa, Temp: %4.2f degC\n", MS5837_Pressure, controlbrd.temperature());
          } else {
            MS5837 = false;
            R = 50;
            G = 0;
            B = 0;
            display.println("MS5837 initilized failed.");
            SerialMon.println("\t\tMS5837 on control board initilized failed.");
          }
          strobeLED(2, 0, 0, 50, 200, 100, 2);        //two blue LED flashes
          strobeLED(2, R, G, B, 100, 200, nDevices);  //nDevices colored strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
          break;
        case BMEADDR:
          nDevices++;
          SerialMon.println("\tControl Board BME280.");
          if (bme.begin()) {
            R = 0;
            G = 50;
            B = 0;
            BME280 = true;
            display.println("BME280 initilized.");
            SerialMon.println("\t\tBME280 initilized.");
            BMEpressure = bme.readPressure();
            reference_pressure = BMEpressure;
            display.printf("Strt: %6.1f Pa, Temp: %4.2f degC, RH: %4.2f %%\n", BMEpressure, bme.readTemperature(), bme.readHumidity());
            SerialMon.printf("\t\tStartup pressure: %6.1f Pa, Temp: %4.2f degC, RH: %4.2f %%\n", BMEpressure, bme.readTemperature(), bme.readHumidity());
          } else {
            BME280 = false;
            R = 50;
            G = 0;
            B = 0;
            display.println("BME280 initilized failed.");
            SerialMon.println("\t\tBME280 initilized failed.");
          }
          strobeLED(2, 0, 0, 50, 50, 50, 2);         //two blue LED flashes
          strobeLED(2, R, G, B, 50, 150, nDevices);  //nDevices colored strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
          break;
        default:
          break;
      }
    } else if (error == 4) {
      SerialMon.print("Unknow error at address 0x");
      if (address < 16) {
        SerialMon.print("0");
      }
      SerialMon.println(address, HEX);
    }
    delay(20);
  }

  display.display();
  TCA9548A(1);
  delay(200);
  SerialMon.println("\nScanning I2C bus with TCA9548 I2C mux set to 1.");
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      SerialMon.print("I2C device found at address 0x");
      if (address < 16) {
        SerialMon.print("0");
      }
      SerialMon.println(address, HEX);
      switch (address) {
        case TCAADDR:
          SerialMon.println("\t\tTCA9548 I2C Mux.");
          break;
        case KELLER_LD_ADDR:  // Keller LD pressure sensor
          SNSRBRD = "Keller_LD9";
          keller.init();
          keller.setFluidDensity(FluidDensity);  // kg/m^3 (freshwater, 1029 for seawater)
          if (keller.isInitialized()) {
            sensorKeller = true;
            nDevices++;
            keller.read();
            kellerTempC = keller.temperature();
            kellerPressure = keller.pressure() * 100;  // pressure in Pa (1mbar - 100 Pa)
            kellerOffset = reference_pressure - kellerPressure;
            SerialMon.println("\t\tKeller Sensor initialized.");
            SerialMon.printf("\t\tStartup pressure: %6.1f Pa, %4.2f degC\n", kellerPressure, kellerTempC);
            kellerDepth = ((kellerPressure - reference_pressure + kellerOffset) / (FluidDensity * 9.80665)) * 3937;
            SerialMon.printf("\t\tStartup depth: %4.2f inches\n", kellerDepth);
            display.printf("Keller: %4.2f, %4.2fF\n", kellerPressure, kellerTempC);
            strobeLED(2, 0, 0, 50, 50, 50, 2);         //two blue LED flashes
            strobeLED(2, R, G, B, 50, 150, nDevices);  //nDevices colored strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
          }
          break;
        case CUI_PS02_ADDR:  // CUI prssure sensor
          SNSRBRD = "CUI_PS02";
          sensorCUI = true;
          nDevices++;
          CUITempC = ps02.getTemperature();
          CUIPressure = ps02.getPressure();  // CUI is gauge pressure in Pa
          CUIOffset = 0 - CUIPressure;
          SerialMon.println("\t\tCUI PS02 Sensor initialized.");
          SerialMon.printf("\t\tStartup pressure: %4.3f Pa gauge, %4.2f degC\n", CUIPressure, CUITempC);
          CUIDepth = ((CUIPressure + CUIOffset) / (FluidDensity * 9.80665)) * 3937;
          SerialMon.printf("\t\tStartup depth: %4.2f inches\n", CUIDepth);
          display.printf("CUI PS02: %4.3fPa gauge, %4.2fC\n", CUIPressure, CUITempC);
          strobeLED(2, 0, 0, 50, 50, 50, 2);         //two blue LED flashes
          strobeLED(2, R, G, B, 50, 150, nDevices);  //nDevices colored strobeLED(LEDs, R, G, B, ms_on, ms_off, times)
        default:
          break;
      }
    } else if (error == 4) {
      SerialMon.print("Unknow error at address 0x");
      if (address < 16) {
        SerialMon.print("0");
      }
      SerialMon.println(address, HEX);
    }
  }
  display.display();
  return nDevices;
}

void TCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);  // TCA9548A address is 0x70
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();
}

void strobeLED(int LEDs, int R, int G, int B, int ms_on, int ms_off, int times) {
  for (int i = 0; i < times; i++) {
    if (LEDs == 0 | LEDs == 2) {
      pixels.setPixelColor(0, pixels.Color(R, G, B));
    } else pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    if (LEDs == 1 | LEDs == 2) {
      pixels.setPixelColor(1, pixels.Color(R, G, B));
    } else pixels.setPixelColor(1, pixels.Color(0, 0, 0));
    pixels.show();
    delay(ms_on);  // Send the updated pixel colors to the hardware.
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
    pixels.show();
    delay(ms_off);  // Send the updated pixel colors to the hardware.
  }
}

void createDataFile() {
  jsonFileName = "/";
  jsonFileName = jsonFileName + String(yr) + '-';
  if (mnth < 10) {
    jsonFileName = jsonFileName + '0';
  }
  jsonFileName = jsonFileName + String(mnth) + '-';
  if (dy < 10) {
    jsonFileName = jsonFileName + '0';
  }
  jsonFileName = jsonFileName + String(dy) + "_00Data.TXT";
  jsonFileName.toCharArray(dataFileChar, 23);
  for (uint8_t i = 0; i < 100; i++) {
    dataFileChar[12] = '0' + i / 10;
    dataFileChar[13] = '0' + i % 10;  // create if does not exist, do not open existing, write, sync after write
    if (!SD.exists(dataFileChar)) {
      break;
    }
  }
  dataFile = SD.open(dataFileChar, FILE_WRITE);
  if (dataFile) {
    SerialMon.printf("Data file %s initialized.\n", dataFileChar);
    lastDay = dy;
  } else {
    SerialMon.println("Error intializing data file.");
  }
}

void readBatVoltage() {
  batteryVoltage = 0.002 * analogReadMilliVolts(V_BAT);  //readBatVoltage();
}

void displayValues() {
  display.clearBuffer();  // Clear the buffer.
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(COLOR1);
  display.println(".  UD/DELDOT LCFS. LAST DATA SET:");
  display.println("Date: " + DateString + ";Time: " + TimeString);
  display.printf("Cntrl=%4.2f C; %6.1f Pa \n", MS5837_0TempC, MS5837_Pressure);
  if (sensorKeller) {
    display.printf("Keller LD9:%4.2fC; %6.1fPa; %3.2fin\n", kellerTempC, kellerPressure, kellerDepth);
  }
  if (sensorCUI) {
    display.printf("CUI PS02:%4.2fC; %4.3fPa gauge; %3.2fin\n", CUITempC, CUIPressure, CUIDepth);
  }
  if (flooded) display.print("flood = TRUE; ");
  else display.print("flood = False; ");
  display.printf("level=%3.2f in.\n", waterlevel);

  display.printf("Solar:%4.3fV,Bat:%4.3fV", inputV, batteryVoltage);
  if (charging) {
    display.println(",charging");
  } else if (done) {
    display.println(",fully charged");
  } else {
    display.println("");
  }
  display.println("   MODEM INFORMATION");
  display.printf("RSSI: %d dBm\n", SIGSTRENGTH);
  if (GPSfix) {  //put in Google maps form (+ = N/E, - = S/W) Ex: +40  42.6142', -74  00.4168'
    display.printf("Lat:%12.6f, Lon:%12.6f\n", GPSlatitude, GPSlongitude);
    display.println("Aquired:" + GPSDate + "," + GPSTime);
  } else {
    display.println("No GPS fix");
  }
  display.println("Modem Resp:" + modemResp);
  display.display();  // actually display all of the above
}
