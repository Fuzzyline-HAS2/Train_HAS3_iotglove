#include "wifi_location.h"

void InitBeetleOta()
{
  beetle_ota.setLogStream(Serial);
  beetle_ota.setOnSkip([]() {
    MySerial1.print("beetle_ota_skip ");
  });
}

void StopBeetleWifi()
{
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
}

String ResolveBeetleManifestUrl()
{
  if (strlen(BEETLE_OTA_MANIFEST_URL) > 0)
  {
    return String(BEETLE_OTA_MANIFEST_URL);
  }

  if (TextEquals(BEETLE_OTA_CHANNEL, "dev"))
  {
    return String(BEETLE_OTA_DEV_MANIFEST_URL);
  }
  if (TextEquals(BEETLE_OTA_CHANNEL, "rc"))
  {
    return String(BEETLE_OTA_RC_MANIFEST_URL);
  }
  return String(BEETLE_OTA_PRD_MANIFEST_URL);
}

bool EnsureWifiConnected()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }

  Serial.println("[OTA] connecting WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(GLOVE_WIFI_SSID, GLOVE_WIFI_PASS);

  unsigned long started_ms = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started_ms < WIFI_CONNECT_TIMEOUT_MS)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  bool connected = WiFi.status() == WL_CONNECTED;
  Serial.println(connected ? "[OTA] WiFi connected" : "[OTA] WiFi connect failed");
  return connected;
}

void RunBeetleOta()
{
  Serial.println("[OTA] command accepted");
  MySerial1.print("beetle_ota_start ");

  if (!EnsureWifiConnected())
  {
    MySerial1.print("beetle_ota_error ");
    StopBeetleWifi();
    return;
  }

  String manifest_url = ResolveBeetleManifestUrl();
  bool ok = beetle_ota.checkManifest(manifest_url.c_str(), BEETLE_OTA_CHANNEL);
  if (!ok)
  {
    MySerial1.print("beetle_ota_error ");
  }

  StopBeetleWifi();
}
