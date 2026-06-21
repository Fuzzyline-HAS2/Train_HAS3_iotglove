#include "wifi_location.h"

#ifndef BEETLE_PIO_BUILD
#include "../SecureOTA.cpp"
#endif

const char BEETLE_BUILD_ID[] =
    "beetle=1"
    ";firmware=" FIRMWARE_VERSION
    ";code=" STRINGIFY(FIRMWARE_VERSION_CODE);

HardwareSerial MySerial1(1);
GameState game_state = setting;

BLEScan *ble_scan = nullptr;
BleCandidate ble_candidates[BLE_MAX_CANDIDATES];
int ble_candidate_count = 0;
String stable_candidate_id;
String last_sent_id;
int stable_candidate_count = 0;

SecureOTA beetle_ota(
    "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/latest/download/beetle-update.bin",
    "https://raw.githubusercontent.com/Fuzzyline-HAS2/updated_IoTglove/main/version.txt",
    "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/latest/download/beetle-update.sig",
    HMAC_SECRET,
    FIRMWARE_VERSION_CODE);

void setup()
{
  Serial.begin(115200);
  MySerial1.begin(115200, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN);
  MySerial1.setTimeout(20);

  pinMode(BEETLE_LED_PIN, OUTPUT);
  digitalWrite(BEETLE_LED_PIN, LOW);

  WiFi.mode(WIFI_OFF);
  InitBleScanner();
  InitBeetleOta();

  Serial.println(BEETLE_BUILD_ID);
  MySerial1.print("reset ");
}

void loop()
{
  ReadTtgoCommands();
  ScanBleLocation();
}
