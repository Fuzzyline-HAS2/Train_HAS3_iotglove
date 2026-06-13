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

//************************************************ Core1 ********************************************************************
/**
 * @brief IoT Glove Intialize
 */
void IotGloveInit()
{
  NextionTftUploadInit();
  MySerial1.begin(115200, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN); // Beetle과 UART 통신 연결 세팅
  nexInit();                                                           // 디스플레이 세팅
  MySerial2.begin(9600, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  // has2wifi.Setup();     // 사무실 와이파이
  // has2wifi.Setup("city"); // 쌈지 시티 와이파이
  WifiForceLowRateInit();                 // 약신호 링크 안정화 (WiFi.begin 전)
  has2wifi.SetDebugPrint(DebugOutput());
  has2wifi.Setup(glove_ssid, glove_pass); // direct WiFi connection
  DebugInit();
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
  NextionTftUploadStartupWindow(4000);
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
  TimerRun();
  BeetleScanWifi();

  if (game_state == activate)
  {
    ActivateFunc();
  }
}
