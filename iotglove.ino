 /**
 * @file iotglove.ino
 * @author YuBin Kim
 * @brief HAS2_iotglove
 * @version 2.1
 * @date 2022-11-16 ~ 2023-01-09
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "iotglove.h"

//************************************************ Core1 ********************************************************************
/**
 * @brief IoT Glove Intialize
 */
void IotGloveInit()
{
  Serial.begin(115200);
  MySerial1.begin(115200, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN);  // Beetle과 UART 통신 연결 세팅
  nexInit();                                                            // 디스플레이 세팅
  MySerial2.begin(9600, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);    // 디스플레이 세팅
  has2wifi.Setup("KT_GiGA_6C64", "ed46zx1198");                         // 내방 사무실 wifi 세팅
  // has2wifi.Setup();                                                  // 쌈지길 매장 wifi 세팅
  SensorInit();                                                         // IoT Glove 사용 센서, 모듈 세팅
  TimerInit();                                                          // 타이머 세팅
  DataChange();
}

/**
 * @brief 아두이노 기본 문법 (전원이 켜지면 한번만 실행)
 */
void setup()
{
  delay(500);
  IotGloveInit();
}

/**
 * @brief 아두이노 기본 문법 (전원이 켜져있는동안 Core1에서 계속 실행)
 */
void loop()
{
  TimerRun();

  switch (game_state) {
  case activate:
    ActivateFunc();
  break;
  
  default:
    break;
  }
}
