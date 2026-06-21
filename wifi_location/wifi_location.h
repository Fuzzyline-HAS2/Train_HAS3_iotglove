#ifndef WIFI_LOCATION_H
#define WIFI_LOCATION_H

#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "../firmware_version.h"
#include "../secrets.h"
#include "../SecureOTA.h"

#define SERIAL1_RX_PIN 6
#define SERIAL1_TX_PIN 5
#define BEETLE_LED_PIN 10

#define BLE_SCAN_SECONDS 3
#define BLE_SCAN_INTERVAL 100
#define BLE_SCAN_WINDOW 99
#define BLE_NAME_PREFIX "HAS2"
#define BLE_STABLE_REQUIRED 2
#define BLE_MAX_CANDIDATES 16

#define WIFI_CONNECT_TIMEOUT_MS 15000

#ifndef BEETLE_OTA_CHANNEL
#define BEETLE_OTA_CHANNEL "dev"
#endif

#ifndef BEETLE_OTA_MANIFEST_URL
#define BEETLE_OTA_MANIFEST_URL ""
#endif

#define BEETLE_OTA_RELEASE_BASE_URL "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/download"
#define BEETLE_OTA_PRD_MANIFEST_URL "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/latest/download/beetle-manifest-prd.json"
#define BEETLE_OTA_DEV_MANIFEST_URL "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/download/dev-latest/beetle-manifest-dev.json"
#define BEETLE_OTA_RC_MANIFEST_URL "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/download/rc-latest/beetle-manifest-rc.json"

#define STRINGIFY_VALUE(value) #value
#define STRINGIFY(value) STRINGIFY_VALUE(value)

enum GameState
{
  setting,
  ready,
  activate
};

struct BleCandidate
{
  String id;
  int rssi_sum;
  int samples;
};

extern const char BEETLE_BUILD_ID[];
extern HardwareSerial MySerial1;
extern GameState game_state;
extern BLEScan *ble_scan;
extern BleCandidate ble_candidates[BLE_MAX_CANDIDATES];
extern int ble_candidate_count;
extern String stable_candidate_id;
extern String last_sent_id;
extern int stable_candidate_count;
extern SecureOTA beetle_ota;

bool TextEquals(const char *value, const char *expected);

void InitBleScanner();
void ResetBleCandidates();
void AddBleSample(const String &id, int rssi);
String BestBleCandidate();
void SendStableLocation(const String &candidate_id);
void ScanBleLocation();

void InitBeetleOta();
void StopBeetleWifi();
bool IsGithubCommand(const String &command);
bool IsValidOtaChannel(const String &channel);
bool IsSafeOtaTag(const String &tag);
void ParseOtaCommand(const String &command, String &channel, String &tag);
String ResolveBeetleManifestUrl(const String &channel, const String &tag);
bool EnsureWifiConnected();
void RunBeetleOta(const String &command);

void HandleTtgoCommand(const String &command);
void ReadTtgoCommands();

#endif
