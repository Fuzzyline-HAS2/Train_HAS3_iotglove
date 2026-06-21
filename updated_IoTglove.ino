/**
 * @file iotglove.ino
 * @author YuBin Kim
 * @brief HAS2_iotglove
 * @version 0.1.0
 * @date 2023-04-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "updated_IoTglove.h"

#define STRINGIFY_VALUE(value) #value
#define STRINGIFY(value) STRINGIFY_VALUE(value)

const char FIRMWARE_BUILD_ID[] =
  "firmware=" FIRMWARE_VERSION
  ";code=" STRINGIFY(FIRMWARE_VERSION_CODE)
  ";git=" BUILD_GIT_COMMIT
  ";esp32=" BUILD_ESP32_CORE
  ";platformio=" BUILD_PLATFORMIO_CORE
  ";platform=" BUILD_PLATFORM
  ";libs=" BUILD_LIBRARY_SUMMARY;

const char *BootWifiStatusText(wl_status_t status)
{
  switch (status)
  {
  case WL_IDLE_STATUS:
    return "idle";
  case WL_NO_SSID_AVAIL:
    return "no_ssid";
  case WL_SCAN_COMPLETED:
    return "scan_completed";
  case WL_CONNECTED:
    return "connected";
  case WL_CONNECT_FAILED:
    return "connect_failed";
  case WL_CONNECTION_LOST:
    return "connection_lost";
  case WL_DISCONNECTED:
    return "disconnected";
  default:
    return "unknown";
  }
}

void BootSerialState(const char *state)
{
  Serial.print("[BOOT] state=");
  Serial.println(state);
}

void BootSerialInfo()
{
  Serial.println();
  Serial.println("[BOOT] IoTGlove starting");
  Serial.print("[BOOT] build=");
  Serial.println(FIRMWARE_BUILD_ID);
  Serial.print("[BOOT] mac=");
  Serial.println(WiFi.macAddress());
  Serial.print("[BOOT] server=http://172.30.1.43/has2.php");
  Serial.println();
}

void BootConnectWifi()
{
  BootSerialState("wifi_connecting");
  Serial.print("[BOOT] ssid=");
  Serial.println(glove_ssid);

  WiFi.begin(glove_ssid, glove_pass);

  unsigned long attempt_started_ms = millis();
  unsigned long last_status_ms = 0;
  unsigned int attempt = 1;

  while (WiFi.status() != WL_CONNECTED)
  {
    unsigned long now = millis();

    if (last_status_ms == 0 || now - last_status_ms >= BOOT_WIFI_STATUS_MS)
    {
      Serial.print("[BOOT] wifi attempt=");
      Serial.print(attempt);
      Serial.print(" elapsed=");
      Serial.print((now - attempt_started_ms) / 1000);
      Serial.print("s status=");
      Serial.print(BootWifiStatusText(WiFi.status()));
      Serial.print(" mac=");
      Serial.println(WiFi.macAddress());
      last_status_ms = now;
    }

    if (now - attempt_started_ms >= BOOT_WIFI_RETRY_MS)
    {
      Serial.print("[BOOT] wifi retry after ");
      Serial.print(BOOT_WIFI_RETRY_MS / 1000);
      Serial.println("s");
      WiFi.disconnect(false);
      delay(BOOT_WIFI_RETRY_PAUSE_MS);
      WiFi.begin(glove_ssid, glove_pass);
      attempt++;
      attempt_started_ms = millis();
      last_status_ms = 0;
    }

    delay(20);
  }

  Serial.print("[BOOT] wifi connected ip=");
  Serial.println(WiFi.localIP());
  Serial.print("[BOOT] debug telnet ");
  Serial.print(WiFi.localIP());
  Serial.print(" ");
  Serial.println(DEBUG_TELNET_PORT);
}

//************************************************ Core1 ********************************************************************
/**
 * @brief IoT Glove Intialize
 */
void IotGloveInit()
{
  NextionTftUploadInit();
  MySerial1.begin(115200, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN); // Beetle과 UART 통신 연결 세팅
  nexInit();                                                           // 디스플레이 세팅
  MySerial1.setTimeout(20);
  MySerial2.begin(9600, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  // has2wifi.Setup();     // 사무실 와이파이
  // has2wifi.Setup("city"); // 쌈지 시티 와이파이
  WifiForceLowRateInit();                 // 약신호 링크 안정화 (WiFi.begin 전)
  BootSerialInfo();
  BootConnectWifi();
  BootSerialState("server_sync");
  has2wifi.SetDebugPrint(DebugOutput());
  has2wifi.Setup(glove_ssid, glove_pass); // direct WiFi connection
  DebugInit();
  Serial.print("[BOOT] telnet ready: telnet ");
  Serial.print(WiFi.localIP());
  Serial.print(" ");
  Serial.println(DEBUG_TELNET_PORT);
  DebugPrint("Build: ");
  DebugPrintln(FIRMWARE_BUILD_ID);
  ota.setLogStream(*DebugOutput());
  ota.setOnSuccess([]() {
    ota_success_blink();
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "setting");
  });
  ota.setOnSkip([]() {
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "setting");
  });
  SensorInit(); // IoT Glove 사용 센서, 모듈 세팅
  TimerInit();  // 타이머 세팅
  has2wifi.Loop();
  DataChange();
}

/**
 * @brief 아두이노 기본 문법 (전원이 켜지면 한번만 실행)
 */
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  delay(500);
  NextionTftUploadInit();
  bool tft_upload_started = NextionTftUploadStartupWindow(NEXTION_TFT_STARTUP_WINDOW_MS);
  NextionTftUploadUseBootSerial();
  BootSerialState(tft_upload_started ? "tft_upload_done" : "tft_upload_skipped");
  IotGloveInit();
}

/** 
 * @brief 아두이노 기본 문법 (전원이 켜져있는동안 Core1에서 계속 실행)
 */
void loop()
{
  if (NextionTftUploadPoll())
  {
    return;
  }

  DebugPoll();
  UpdateBeetleOtaFlow();
  UpdateSurConnectFlow();
  TimerRun();
  UpdateVibration();
  BeetleScanWifi();

  if (game_state == activate)
  {
    ActivateFunc();
  }
}
