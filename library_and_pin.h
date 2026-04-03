#ifndef _LIBRARY_AND_PIN_H_
#define _LIBRARY_AND_PIN_H_

// 라이브러리 선언
#include <Esp.h>

#include <HAS2_Wifi.h>

#include <Adafruit_NeoPixel.h>
#include <NexHardware.h>             // 디스플레이 관련 라이브러리
#include <Pangodream_18650_CL.h> // 배터리 체크 관련 라이브러리

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

#include <SimpleTimer.h>
#include <esp_task_wdt.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <SecureOTA.h>
#include "secrets.h"

#define FIRMWARE_VER 6

// 핀 선언
#define SERIAL1_RX_PIN 36 // ESP32 비틀 RX
#define SERIAL1_TX_PIN 32 // ESP32 비틀 TX

#define SERIAL2_RX_PIN 39 // 디스플레이 RX
#define SERIAL2_TX_PIN 33 // 디스플레이 TX

#define NEOPIXEL_PIN 25 // 네오픽셀 N구 1개

#define IR_RECEIVE_PIN 27 // IR 수신부 1개
#define IR_SEND_PIN 4     // IR 송신부 1개

#define BUZZER_PIN 15 // 피에조부저 1개

#define MOTOR_INA1_PIN 14 // 동전형 진동모터 3개중 1개
#define MOTOR_PWMA_PIN 12 // 동전형 진동모터 3개중 1개
#define MOTOR_INA2_PIN 13 // 동전형 진동모터 3개중 1개

#define QRD1114_PIN 34 // 근접센서 1개

#define ADC_PIN 35      // 배터리 체크 관련 (선연결 X)
#define CONV_FACTOR 1.7 // 배터리 체크 관련 상수
#define READS 20        // 배터리 체크 관련 상수

#endif