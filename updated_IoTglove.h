#define __DEBUG__

#ifndef _IOTGLOVE_H_
#define _IOTGLOVE_H_

#include "library_and_pin.h"

//============================ Global Variable ============================
String wifi_name;

bool hacking_state;
bool hacking;
int hack_count = 0;
#define HACK_THRESHOLD 3
bool revival;
bool breath_hold;
bool motor_on;
bool pending_revival_vibration;
String ir_decode_data;
char current_hmi_page[20];

#define REVIVAL_HELP_RECORDS 10
#define REVIVAL_TICK_PER_SEC 10
#define REVIVAL_FINISH_DISPLAY_MS 1500
#define TAG_CAPTURE_MIN_MS 3000
#define TAG_CAPTURE_TIMEOUT_MS 5000
#define SUR_CONNECT_MIN_MS 3000
#define SUR_CONNECT_TIMEOUT_MS 5000
#define PAGE_CHANGE_VIBRATION_ON_MS 120
#define PAGE_CHANGE_VIBRATION_GAP_MS 80
#define PAGE_CHANGE_VIBRATION_TOTAL_MS (PAGE_CHANGE_VIBRATION_ON_MS * 2 + PAGE_CHANGE_VIBRATION_GAP_MS)
#define PAGE_CHANGE_VIBRATION_INTENSITY 3
#define DEFAULT_REVIVAL_TIME_SEC 30
#define DEBUG_TELNET_PORT 23
#define ROLE_SEND_RETRY_MS 2000

struct RevivalHelpRecord
{
    String device_name;
    unsigned long expires_at;
};

RevivalHelpRecord revival_help_records[REVIVAL_HELP_RECORDS];
bool revival_timer_active = false;
bool revival_finish_page = false;
bool revival_finish_sent = false;
unsigned long revival_started_ms = 0;
unsigned long revival_bonus_ms = 0;
unsigned long revival_finish_started_ms = 0;
unsigned long last_role_send_ms = 0;
int last_revival_cooldown_count = 0;
bool tag_capture_pending = false;
unsigned long tag_capture_started_ms = 0;
bool sur_connect_pending = false;
unsigned long sur_connect_started_ms = 0;
bool page_change_vibration_active = false;
unsigned long page_change_vibration_started_ms = 0;

typedef enum GameState
{
    setting,
    ready,
    activate
};
GameState game_state = setting;

//============================ Hardware Serial ============================
HardwareSerial MySerial1(1); // Beetle
HardwareSerial MySerial2(2); // Display

//=============================== Display ===============================
void DisplaySet();
void DisplayCheck();
void PageChange(const char *page);
void SendHmiValue(const char *target, int value);
void UpdateHmiLanguage();
void UpdateHmiBattery();
void UpdateHmiResults();
void AddRevivalGaugeBonus(int seconds);

//========================== Nextion TFT Upload ==========================
void NextionTftUploadInit();
bool NextionTftUploadStartupWindow(unsigned long window_ms);
bool NextionTftUploadPoll();
void NextionTftUploadRestoreDisplaySerial();

//=============================== Debug ===============================
void DebugInit();
void DebugPoll();
Print *DebugOutput();
void DebugPrint(const char *value);
void DebugPrint(const String &value);
void DebugPrint(unsigned long value);
void DebugPrintln();
void DebugPrintln(const char *value);
void DebugPrintln(const String &value);
void DebugPrintln(unsigned long value);
void DebugPrintln(unsigned long value, int base);
void DebugPrintf(const char *format, ...);

extern const char FIRMWARE_BUILD_ID[];

//*=============================== Sensor ===============================*
/**
 * @brief IoT 글러브에 사용되는 센서, 모듈 세팅
 */
void SensorInit();

//================================ Wifi ==================================
#include <esp_wifi.h>

// WiFi direct connection target. Keep actual values in local secrets.h.
#ifndef GLOVE_WIFI_SSID
#define GLOVE_WIFI_SSID "badland"
#endif

#ifndef GLOVE_WIFI_PASS
#define GLOVE_WIFI_PASS ""
#endif

static char glove_ssid[] = GLOVE_WIFI_SSID;
static char glove_pass[] = GLOVE_WIFI_PASS;

// 약신호 링크 안정화(최대 TX 파워 / 모뎀 슬립 off / 자동 재연결). WiFi.begin 전에 호출.
void WifiForceLowRateInit();

HAS2_Wifi has2wifi("http://172.30.1.43");

//================================ OTA ==================================
#define OTA_PRD_MANIFEST_URL "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/latest/download/manifest-prd.json"
#define OTA_DEV_MANIFEST_URL "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/download/v1.2.4-dev.1/manifest-dev.json"
#define OTA_RC_MANIFEST_URL "https://github.com/Fuzzyline-HAS2/updated_IoTglove/releases/download/v1.2.4-rc.1/manifest-rc.json"

SecureOTA ota(
  "https://raw.githubusercontent.com/Fuzzyline-HAS2/updated_IoTglove/main/update.bin",
  "https://raw.githubusercontent.com/Fuzzyline-HAS2/updated_IoTglove/main/version.txt",
  "https://raw.githubusercontent.com/Fuzzyline-HAS2/updated_IoTglove/main/update.sig",
  HMAC_SECRET,
  FIRMWARE_VERSION_CODE
);

bool activate_bool;

void SettingFunc();
void ReadyFunc();
void ActivateFunc();
void ActivateRunOnce();
void DataChange();
bool TextEquals(const char *value, const char *expected);
bool IsPlayerRole(const char *role);
bool IsTaggerRole(const char *role);
bool IsRevivalRole(const char *role);
bool IsRevivalRole(const String &role);
void ResetRevivalTimer();
void StartRevivalTimer();
void UpdateRevivalTimer();
void UpdateTagCaptureFlow();
void StartSurConnect();
void StopSurConnect();
void UpdateSurConnectFlow();
void ShowRolePage();
void ApplyRevivalCooldownChange(int previous_count);
bool IsFinalLifeTaken();
void StartPendingRevivalVibration();
void StopPendingRevivalVibration();
void StartPageChangeVibration();
void UpdateVibration();

//=============================== Neopixel ===============================
#define NUMPIXELS 4
#define DEFAULT_BRIGHTNESS 50

int ledBrightness = DEFAULT_BRIGHTNESS;
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Neopixel 색상정보
int white[3]  = {255, 255, 255};
int red[3]    = {255, 0,   0  };
int yellow[3] = {255, 255, 0  };
int green[3]  = {0,   255, 0  };
int skyblue[3] = {0,   180, 255};
int purple[3] = {255, 0,   255};
int blue[3]   = {0,   0,   255};

void lightColor(int color[]);
void UpdateBrightness();

//================================== IR ==================================
#define DECODE_NEC_
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECEIVE_PIN);

decode_results results;

uint64_t ir_send_data = 0;
bool ir_receive_error = false;

void IrInit();
void IrSendDataSetup(String device_name);
void IrSend();
void IrReceive();
String IrDecoding(uint32_t ir_data);
void ClearRevivalHelpRecords();
bool ShouldSendRevivalCooldown(String device_name, unsigned long ttl_ms);

//=========================== Vibration Motor ===========================
const int MotorFreq = 5000;
const int MotorResolution = 8;
const int MotorLedChannel = 3;
const int VibrationLedChannel = 4;
const int MotorMAX_DUTY_CYCLE = (int)(pow(2, MotorResolution) - 1);

#define ARRAYINDEX(X) sizeof(X) / sizeof(int)

#define vibration_mode0 0
#define vibration_mode1 130
#define vibration_mode2 180
#define vibration_mode3 210
#define vibration_mode4 230
#define vibration_mode5 240

const int vibration_pattern_1[] = {1, 0, 0, 1, 5, 2, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0}; // BPM 83
const int vibration_pattern_2[] = {1, 0, 1, 5, 3, 0, 0, 1, 2, 1, 0, 0};                   // BPM 125
const int vibration_pattern_3[] = {5, 5, 5, 5, 5, 5, 5};
const int vibration_pattern_4[] = {1, 2, 1, 0, 0, 1, 5, 2, 1, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // BPM 56

void MotorInit();
int Intensity(int intensity);
void MotorOn(const int *vibration_pattern, int len);
void MotorStop();

//=============================== QRD1114 ===============================
/**
 * @brief QRD1114 근접센서를 통한 근접거리 측정
 *
 * @return float 측정값 (0.1 ~ 20.1 [mm])
 */
float DistanceCheck();

//=============================== Battery ===============================
Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

void BatteryCheck();

//================================ Timer ================================
// 타이머 사용 선언
SimpleTimer ir_receive_timer; // ir 수신 타이머
SimpleTimer player_ir_send_timer;
SimpleTimer hacking_timer; // ir 수신 타이머
SimpleTimer wifi_timer;
SimpleTimer neopixel_timer;
SimpleTimer battery_timer;

void TimerInit();
void TimerRun();
void WifiTimerFunc();

// 타이머 설정 관련 함수
int ir_receive_timer_id;
int player_ir_send_timer_id;
int hacking_timer_id;
int wifi_timer_id;
int neopixel_timer_id;
int battery_timer_id;

//================================ Neo =================================
void ota_success_blink();
void tagger_blink();
#endif
