boolean Skywire() {
  unsigned long start;
  String data;
  int year3 = 0;
  int month3 = 0;
  int day3 = 0;
  int hour3 = 0;
  int min3 = 0;
  int sec3 = 0;
  float timezone = 0;
  float lat2 = 0;
  float lon2 = 0;
  float speed2 = 0;
  float alt2 = 0;
  int vsat2 = 0;
  int usat2 = 0;
  float accuracy2 = 0;
  int year2 = 0;
  int month2 = 0;
  int day2 = 0;
  int hour2 = 0;
  int min2 = 0;
  int sec2 = 0;


  if (!modem.isNetworkConnected()) {  // if already connected skip initialization
    //pinMode(MODEM_STATUS, INPUT);                                 // Configure I/O pin 19 as input
    if (!digitalRead(MODEM_STATUS)) {  // Skywire modem is off so turn it on
      SerialMon.println("Turning on Modem.");
      display.println("Turning on Modem.");
      display.display();
      //digitalWrite(SW_MODEM_ON_OFF, LOW);                         // Turn off internal PU resistor
      //pinMode(SW_MODEM_ON_OFF, OUTPUT);                           // Configure I/O pin 21 as output
      digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn on Skywire modem
      delay(500);                          // modem requires >100ms pulse
      digitalWrite(SW_MODEM_ON_OFF, HIGH);
      //pinMode(SW_MODEM_ON_OFF, INPUT);                            // Return I/O pin 21 to input/hi-Z state
    }
    SerialAT.begin(115200);
    //TinyGsmAutoBaud(SerialAT, 9600, 115200);
    if (startup) {  //first time thru so restart instead of initialize the cellular modem
      SerialMon.println("Restarting Skywire module.");
      SerialMon.println("Initializing modem COM port");
      display.println("Restarting Skywire module.");
      display.println("Initializing modem COM port");
      display.display();
      if (!modem.restart()) {
        SerialMon.println("Failed to restart modem, delaying 10s and retrying");
        display.println("Failed modem restart,delay 10s,retry");
        display.display();
        modem.poweroff();
        delay(2000);
        //pinMode(MODEM_RESET, GPIO_MODE_OUTPUT_OD);                // Make modem rest pin open drain
        digitalWrite(MODEM_RESET, LOW);  // Force reset Skywire modem
        delay(500);                      // modem requires >150ms pulse
        digitalWrite(MODEM_RESET, HIGH);
        delay(2000L);
        if (!digitalRead(MODEM_STATUS)) {  // Skywire modem is off so turn it on
          //digitalWrite(SW_MODEM_ON_OFF, LOW);                     // Turn off internal PU resistor
          //pinMode(SW_MODEM_ON_OFF, OUTPUT);                       // Configure I/O pin 21 as output
          digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn on Skywire modem
          delay(500);                          // modem requires >100ms pulse
          digitalWrite(SW_MODEM_ON_OFF, HIGH);
          //pinMode(SW_MODEM_ON_OFF, INPUT);                        // Return I/O pin 21 to input/hi-Z state
        }
        delay(2000L);
        TinyGsmAutoBaud(SerialAT, 9600, 115200);
        if (!modem.init()) {
          SerialMon.println("Failed to restart modem. Powering modem off.");
          display.println("Failed modem restart.");
          display.display();
          //NoteString = NoteString + ", failed modem restart";
          if (digitalRead(MODEM_STATUS)) {  // Skywire modem is on so turn it off
            modem.poweroff();
            //digitalWrite(SW_MODEM_ON_OFF, LOW);                     // Turn off internal PU resistor
            //pinMode(SW_MODEM_ON_OFF, OUTPUT);                       // Configure I/O pin 21 as output
            digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn on Skywire modem
            delay(1000);                         // modem requires >650ms pulse
            digitalWrite(SW_MODEM_ON_OFF, HIGH);
            //pinMode(SW_MODEM_ON_OFF, INPUT);                        // Return I/O pin 21 to input/hi-Z state
          }
          return false;
        }
      }
      String name = modem.getModemName();
      SerialMon.println("Modem Name: " + name);
      display.println("Modem Name: " + name);
      String modemInfo = modem.getModemInfo();
      MODEMTYPE = modemInfo.substring(modemInfo.indexOf(":") + 2);
      SerialMon.println("Modem Rev: " + MODEMTYPE);
      display.println("Modem Info: " + modemInfo);
      display.display();
    } else {  //use modem.init to speed things up
      SerialMon.println("Initializing Skywire module.");
      SerialMon.println("Initializing modem COM port");
      display.println("Initializing Skywire module.");
      display.println("Initializing modem COM port");
      if (!modem.init()) {
        SerialMon.println("Failed to init modem, delaying 10s and retrying");
        display.println("Failed modem restart,delay 10s,retry");
        display.display();
        modem.poweroff();
        delay(2000);
        if (!digitalRead(MODEM_STATUS)) {  // Skywire modem is off so turn it on
          //digitalWrite(SW_MODEM_ON_OFF, LOW);                     // Turn off internal PU resistor
          //pinMode(SW_MODEM_ON_OFF, OUTPUT);                       // Configure I/O pin 21 as output
          digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn on Skywire modem
          delay(250);                          // modem requires >100ms pulse
          digitalWrite(SW_MODEM_ON_OFF, HIGH);
          //pinMode(SW_MODEM_ON_OFF, INPUT);                        // Return I/O pin 21 to input/hi-Z state
        }
        delay(2000L);
        TinyGsmAutoBaud(SerialAT, 9600, 115200);
        if (!modem.init()) {
          SerialMon.println("Failed to restart modem. Poowering modem off.");
          display.println("Failed modem restart.");
          display.display();
          //NoteString = NoteString + ", failed modem restart";
          if (digitalRead(MODEM_STATUS)) {  // Skywire modem is on so turn it off
            modem.poweroff();
            //digitalWrite(SW_MODEM_ON_OFF, LOW);                     // Turn off internal PU resistor
            //pinMode(SW_MODEM_ON_OFF, OUTPUT);                       // Configure I/O pin 21 as output
            digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn on Skywire modem
            delay(1000);                         // modem requires >650ms pulse
            digitalWrite(SW_MODEM_ON_OFF, HIGH);
            //pinMode(SW_MODEM_ON_OFF, INPUT);                        // Return I/O pin 21 to input/hi-Z state
          }
          return false;
        }
      }
    }
    SerialMon.println("Waiting for network...");
    display.println("Waiting for network...");
    display.display();
    start = millis();
    while (!modem.waitForNetwork(60000L)) {
      delay(2000);
      SerialMon.print(".");
      if (millis() - start > 120000L) {  //give it up to 2 minutes to connect to Verizon network
        connectNetwork = false;
        break;
      }
    }
  }
  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
    display.println("Network connected");
    SerialMon.println("Connecting to " + (String)apn);
    display.clearDisplay();  // Clear the buffer.
    display.setCursor(0, 0);
    display.println("Connecting to " + (String)apn);
    start = millis();
    while (!modem.gprsConnect(apn)) {  //, gprsUser, gprsPass)) {
      delay(2000);
      SerialMon.print(".");
      if (millis() - start > 60000L) {
        connectNetwork = false;
        break;
      }
    }
    if (modem.isGprsConnected()) {
      SerialMon.println("GPRS status: connected");
      display.println("GPRS status: connected");
      connectNetwork = true;
      if (startup) {
        String ccid = modem.getSimCCID();
        SIMCCID = ccid;
        SerialMon.println("CCID: " + ccid);
        display.println("CCID: " + ccid);
        String imei = modem.getIMEI();
        DEVICE_ID = imei;
        SerialMon.println("IMEI: " + imei);
        display.println("IMEI: " + imei);
        String imsi = modem.getIMSI();
        SerialMon.println("IMSI: " + imsi);
        display.println("IMSI: " + imsi);
        String cop = modem.getOperator();
        SerialMon.println("Operator: " + cop);
        display.println("Operator: " + cop);
        MODEMTECH = "CAT_M1";
        IPAddress local = modem.localIP();
        SerialMon.println("Local IP: " + String(local));
        display.println("Local IP: " + String(local));
      }
      int csqval = modem.getSignalQuality();
      if (csqval == 99) SIGSTRENGTH = -999;
      else SIGSTRENGTH = -113 + (2 * csqval);  // convert to dBm signal strength
      SerialMon.print("Signal quality: ");
      display.print("Signal quality: ");
      SerialMon.println(SIGSTRENGTH);
      display.println(SIGSTRENGTH);
      MODEMTEMP = modem.getTemperature();
      SerialMon.println("Chip temperature: " + String(MODEMTEMP));
      display.println("Chip temperature: " + String(MODEMTEMP));
      display.display();
      if (getNetworkTime) {
        SerialMon.println("Requesting current network time");
        display.println("Requesting current network time");
        for (int8_t i = 5; i; i--) {
          if (modem.getNetworkTime(&year3, &month3, &day3, &hour3, &min3, &sec3, &timezone)) {
            yr = year3;
            mnth = month3;
            dy = day3;
            secs = sec3;
            mins = min3;
            hr = hour3;
            setTime(hr, mins, secs, dy, mnth, yr);
            NetTimeString = "";
            NetDateString = String(yr) + "/";
            if (mnth < 10) NetDateString = NetDateString + "0";
            NetDateString = NetDateString + String(mnth) + "/";
            if (dy < 10) NetDateString = NetDateString + "0";
            NetDateString = NetDateString + String(dy);
            if (hr < 10) NetTimeString = "0";
            NetTimeString = NetTimeString + String(hr) + ":";
            if (mins < 10) NetTimeString = NetTimeString + "0";
            NetTimeString = NetTimeString + String(mins) + ":";
            if (secs < 10) NetTimeString = NetTimeString + "0";
            NetTimeString = NetTimeString + String(secs);
            SerialMon.printf("Cellular Local Network Time: %s; %s\n", NetTimeString, NetDateString);
            display.printf("Cellular Local Network Time:\n%s; %s\n", NetTimeString, NetDateString);
            //NoteString = NoteString + ", Network Time:" + NetTimeString + ";" + NetDateString;
            getNetworkTime = false;
            validDate = true;
            break;
          } else {
            SerialMon.print(".");
            delay(6000L);
          }
        }
        if (getNetworkTime) {
          SerialMon.println("Couldn't get network time.");
          display.println("Couldn't get network time.");
          //NoteString = NoteString + ", no network time";
        }
      }
      getNetworkTime = true;
      if (getLocation) {
        SerialMon.println("Requesting current GPS/GNSS/GLONASS location");
        display.println("Request current GPS location");
        SerialMon.println("Enabling GPS/GNSS/GLONASS and waiting 15s for warm-up");
        modem.enableGPS();
        delay(15000L);
        if (modem.getGPS(&lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2, &year2, &month2, &day2, &hour2, &min2, &sec2)) {
          GPSlatitude = lat2;
          GPSlongitude = lon2;
          GPSaltitidue = alt2;
          GPSsatellites = vsat2;
          GPSaccuracy = accuracy2;
          getLocation = false;
          GPSstartup = false;
          GPSDate = NetDateString;
          GPSTime = NetTimeString;
        } else {
          if (GPSstartup) {
            display.print("On startup retry GPS");
            SerialMon.print("On startup retry GPS");
            for (int8_t i = 5; i; i--) {
              if (modem.getGPS(&lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2, &year2, &month2, &day2, &hour2, &min2, &sec2)) {
                GPSlatitude = lat2;
                GPSlongitude = lon2;
                GPSaltitidue = alt2;
                GPSsatellites = vsat2;
                GPSaccuracy = accuracy2;
                getLocation = false;
                GPSDate = NetDateString;
                GPSTime = NetTimeString;
                break;
              }
              SerialMon.print(".");
              display.print(".");
              delay(4000);
            }
          }
        }
        if (getLocation) {  // was not able to get GPS location
          SerialMon.println("\nGPS Initialized failed.");
          display.println("\nGPS Init failed.");
        } else {  //aquired new gps location info
          SerialMon.printf("\nGPS Lat:%12.8f, Lon:%12.8f\n", GPSlatitude, GPSlongitude);
          SerialMon.printf("GPS satellites: %d, accuracy: %4.2f\n", GPSsatellites, GPSaccuracy);
          display.printf("\nGPS Lat:%12.8f, Lon:%12.8f\n", GPSlatitude, GPSlongitude);
          display.printf("GPS satellites: %d, accuracy: %4.2f\n", GPSsatellites, GPSaccuracy);
          LocationString = "Lat:" + String(GPSlatitude, 8) + ", Lon:" + String(GPSlongitude, 8);
          GPSfix = true;
          eepromData.begin("my-data", false);  //setup namespace for nonvolatile R/W mode data storage
          eepromData.putFloat("lat", GPSlatitude);
          eepromData.putFloat("lon", GPSlongitude);
          eepromData.end();
        }
        SerialMon.println("Disabling GPS");
        display.println("Disabling GPS");
        modem.disableGPS();
        display.display();
      }  //end getLocation that aquires GPS location
      if (POST_DATA) {
        SerialMon.println("Connecting to " + (String)server);
        if (!client.connect(server, port)) {
          SerialMon.println("... failed");
          return false;
        } else {  // Make a HTTP POST:
          SerialMon.print("POSTING: ");
          SerialMon.println(jsondata);
          client.print(String("POST ") + resource + " HTTP/1.0\r\n");
          client.print(String("Host: ") + server + "\r\n");
          client.print(String("Content-Length: ") + jsondata.length() + "\r\n");
          client.print(String("Content-Type:application/x-www-form-urlencoded\r\n"));
          client.print(String("\r\n") + jsondata);  //
          client.print("Connection:close\r\n\r\n");

          // Wait for data to arrive
          start = millis();
          while (client.connected() && !client.available() && millis() - start < 30000L) {
            delay(100);
          };
          // Read data
          modemResp = "";
          bool read1 = true;
          start = millis();
          while (client.connected() && millis() - start < 5000L) {
            while (client.available()) {
              int modemchar = client.read();
              char lastchar = char(modemchar);
              if (isAscii(lastchar)) {
                modemResp = modemResp + lastchar;
              }
              start = millis();
            }
          }
          SerialMon.println("\nmodem response:");
          SerialMon.println(modemResp);
          SerialMon.println(":end modem response");
        }
      }
      client.stop();
      SerialMon.println("Poweroff() modem.");
      modem.poweroff();
      delay(1000);
      if (digitalRead(MODEM_STATUS)) {  // Skywire modem is on so turn it off
        SerialMon.println("Turn off modem using SW_MODEM_ON_OFF pin.");
        //digitalWrite(SW_MODEM_ON_OFF, LOW);                     // Turn off internal PU resistor
        //pinMode(SW_MODEM_ON_OFF, OUTPUT);                       // Configure I/O pin 21 as output
        digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn off Skywire modem
        delay(1000);                         // modem requires >650ms pulse
        digitalWrite(SW_MODEM_ON_OFF, HIGH);
        //pinMode(SW_MODEM_ON_OFF, INPUT);                        // Return I/O pin 21 to input/hi-Z state
      }
      startup = false;
      display.display();
      return true;
    } else {
      SerialMon.println("GPRS status: not connected");
      display.println("GPRS status: not connected");
      SerialMon.println("Poweroff() modem.");
      modem.poweroff();
      delay(1000);
      if (digitalRead(MODEM_STATUS)) {  // Skywire modem is on so turn it off
        SerialMon.println("Turn off modem using SW_MODEM_ON_OFF pin.");
        //digitalWrite(SW_MODEM_ON_OFF, LOW);                     // Turn off internal PU resistor
        //pinMode(SW_MODEM_ON_OFF, OUTPUT);                       // Configure I/O pin 21 as output
        digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn on Skywire modem
        delay(1000);                         // modem requires >650ms pulse
        digitalWrite(SW_MODEM_ON_OFF, HIGH);
        //pinMode(SW_MODEM_ON_OFF, INPUT);                        // Return I/O pin 21 to input/hi-Z state
      }
      startup = true;
      getNetworkTime = true;
      display.display();
      return false;
    }
  } else {
    SerialMon.println("Network not connected");
    display.println("Network not connected");
    display.display();
    SerialMon.println("Poweroff() modem.");
    modem.poweroff();
    delay(1000);
    if (digitalRead(MODEM_STATUS)) {  // Skywire modem is on so turn it off
      SerialMon.println("Turn off modem using SW_MODEM_ON_OFF pin.");
      //digitalWrite(SW_MODEM_ON_OFF, LOW);                     // Turn off internal PU resistor
      //pinMode(SW_MODEM_ON_OFF, OUTPUT);                       // Configure I/O pin 21 as output
      digitalWrite(SW_MODEM_ON_OFF, LOW);  // Turn on Skywire modem
      delay(1000);                         // modem requires >650ms pulse
      digitalWrite(SW_MODEM_ON_OFF, HIGH);
      //pinMode(SW_MODEM_ON_OFF, INPUT);                        // Return I/O pin 21 to input/hi-Z state
    }
    startup = true;
    getNetworkTime = true;
    connectNetwork = false;
    return false;
  }
}

//**********************************************************************************************************************