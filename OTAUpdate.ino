/***************************************************************************************
      Over the air (OTA) firmware update over the Skywire cellular connection.
      The modem must already be connected (modem.isGprsConnected()) before calling.
      Server must host:
            otaVersionResource  - text file "<version> <md5>", ex: "Rev2.63 9f1c2e..."
            otaFirmwareResource - compiled sketch binary (.ino.bin)
      If the server version differs from FWVERSION the new image is downloaded to the
      spare OTA partition, verified with MD5, and the ESP32 reboots into it.
      Returns false if no update was needed or the update failed (running firmware
      is left untouched). Does not return if the update succeeds.
**************************************************************************************/
#include <Update.h>

boolean checkForOTA() {
  unsigned long start;
  long contentLength, written;
  int statusCode;
  uint8_t buf[512];

  if (!modem.isGprsConnected()) {
    SerialMon.println("OTA: modem not connected, skipping.");
    return false;
  }

  // Get latest firmware version and MD5 from server
  SerialMon.println("OTA: checking for firmware update.");
  if (http.get(otaVersionResource) != 0) {
    SerialMon.println("OTA: failed to connect to server.");
    http.stop();
    return false;
  }
  statusCode = http.responseStatusCode();
  if (statusCode != 200) {
    SerialMon.printf("OTA: version check HTTP status %d.\n", statusCode);
    http.stop();
    return false;
  }
  String body = http.responseBody();
  http.stop();
  body.trim();
  int space = body.indexOf(' ');
  if (space < 0) {
    SerialMon.println("OTA: bad version file: " + body);
    return false;
  }
  String newVersion = body.substring(0, space);
  String md5 = body.substring(space + 1);
  md5.trim();
  SerialMon.println("OTA: running " + FWVERSION + ", server has " + newVersion);
  if (newVersion == FWVERSION) {
    SerialMon.println("OTA: firmware is up to date.");
    return false;
  }
  if (md5.length() != 32) {
    SerialMon.println("OTA: bad MD5 in version file: " + md5);
    return false;
  }
  batteryVoltage = 0.002 * analogReadMilliVolts(V_BAT);
  if (batteryVoltage < OTA_MIN_BATTERY_V) {
    SerialMon.printf("OTA: battery %4.2fV too low, skipping update.\n", batteryVoltage);
    return false;
  }

  // Download new firmware into the spare OTA partition
  SerialMon.println("OTA: downloading " + newVersion);
  display.println("OTA update to " + newVersion);
  display.display();
  if (http.get(otaFirmwareResource) != 0) {
    SerialMon.println("OTA: failed to connect to server.");
    http.stop();
    return false;
  }
  statusCode = http.responseStatusCode();
  if (statusCode != 200) {
    SerialMon.printf("OTA: firmware download HTTP status %d.\n", statusCode);
    http.stop();
    return false;
  }
  contentLength = http.contentLength();  // also skips the response headers
  if (contentLength <= 0) {
    SerialMon.println("OTA: server did not send Content-Length.");
    http.stop();
    return false;
  }
  if (!Update.begin(contentLength)) {
    SerialMon.printf("OTA: not enough space for %ld bytes: %s\n", contentLength, Update.errorString());
    http.stop();
    return false;
  }
  Update.setMD5(md5.c_str());  // Update.end() fails if the downloaded image does not match

  written = 0;
  start = millis();
  while (written < contentLength && millis() - start < OTA_TIMEOUT_MS) {
    int n = http.read(buf, sizeof(buf));
    if (n > 0) {
      if (Update.write(buf, n) != (size_t)n) {
        SerialMon.printf("OTA: flash write failed: %s\n", Update.errorString());
        break;
      }
      written += n;
      start = millis();
      if ((written % 51200) < n) SerialMon.printf("OTA: %ld / %ld bytes\n", written, contentLength);  // progress every ~50KB
    } else {
      delay(10);
    }
  }
  http.stop();

  if (written != contentLength) {
    SerialMon.printf("OTA: download incomplete (%ld / %ld bytes).\n", written, contentLength);
    Update.abort();
    display.println("OTA update failed.");
    display.display();
    return false;
  }
  if (!Update.end()) {  // checks size and MD5, then marks new partition as boot partition
    SerialMon.printf("OTA: verify failed: %s\n", Update.errorString());
    display.println("OTA update failed.");
    display.display();
    return false;
  }

  // Success - clean up and reboot into new firmware
  SerialMon.println("OTA: update successful, rebooting into " + newVersion);
  display.println("OTA success, rebooting.");
  display.display();
  if (LogDataSDCard && validFile) {
    dataFile.close();  // make sure logged data is written to SD card before reboot
  }
  modem.poweroff();
  delay(1000);
  SerialMon.flush();
  esp_restart();
  return true;
}
