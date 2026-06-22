#include "updated_IoTglove.h"
//****************************************** Initialize ******************************************
void SensorInit()
{
  // Neopixel init
  pixels.begin();
  pixels.setBrightness(ledBrightness);
  lightColor(white);

  // IR init
  IrInit();

  // Vibration moter init
  MotorInit();

  // Buzzer init
  ledcAttach(BUZZER_PIN, MotorFreq, MotorResolution);
  ledcWrite(BUZZER_PIN, 0);

  // QRD1114 init
  pinMode(QRD1114_PIN, INPUT);
}
//********************************************** IR **********************************************
/**
 * @brief IR Receiver, Transmitter 세팅
 */
void IrInit()
{
  irsend.begin();
  irrecv.enableIRIn(); // Start the receiver
  IrSendDataSetup((String)(const char *)my["device_name"]);
}

/**
 * @brief IR 송신 데이터 설정
 * @param device_name MYSQL에 저장되어 있는 디바이스 이름 => (const char*)my["device_name"]
 */
void IrSendDataSetup(String device_name)
{
  ir_send_data = 0;

  // Address : 그룹(G) command : 플레이어(P) 에 해당하는 숫자
  // 1. String 변수를 char 배열형식으로 변경
  int str_len = device_name.length() + 1;
  char char_device_name[str_len];
  device_name.toCharArray(char_device_name, str_len);

  // 2. 데이터 나누기
  byte address = char_device_name[1] - '0'; // char형 변수에서 int 값을 얻기위해 -'0'
  byte command = char_device_name[3] - '0';
  byte address_bar = ~address;
  byte command_bar = ~command;
  // IR data가 올바르게 전송되는지 확인하기 위해 _bar 변수 생성

  ir_send_data = ir_send_data ^ address;
  ir_send_data = ir_send_data << 8 ^ address_bar;
  ir_send_data = ir_send_data << 8 ^ command;
  ir_send_data = ir_send_data << 8 ^ command_bar;

  DebugPrint("ir_send_data : ");
  DebugPrintln((unsigned long)ir_send_data, HEX);
}

/**
 * @brief IR 송신
 */
void IrSend()
{
  unsigned long now = millis();
  if ((long)(now - last_ir_send_ms) < IR_SEND_INTERVAL_MS)
  {
    return;
  }

  last_ir_send_ms = now;
  irsend.sendNEC(ir_send_data);
}

void ClearRevivalHelpRecords()
{
  for (int i = 0; i < REVIVAL_HELP_RECORDS; i++)
  {
    revival_help_records[i].device_name = "";
    revival_help_records[i].expires_at = 0;
  }
}

bool HelpRecordExpired(unsigned long now, unsigned long expires_at)
{
  return expires_at == 0 || (long)(now - expires_at) >= 0;
}

bool ShouldSendRevivalCooldown(String device_name, unsigned long ttl_ms)
{
  if (device_name.length() == 0)
  {
    return false;
  }

  unsigned long now = millis();
  for (int i = 0; i < REVIVAL_HELP_RECORDS; i++)
  {
    if (revival_help_records[i].device_name.length() > 0 && HelpRecordExpired(now, revival_help_records[i].expires_at))
    {
      revival_help_records[i].device_name = "";
      revival_help_records[i].expires_at = 0;
    }
  }

  for (int i = 0; i < REVIVAL_HELP_RECORDS; i++)
  {
    if (revival_help_records[i].device_name == device_name)
    {
      return false;
    }
  }

  int target_index = 0;
  for (int i = 0; i < REVIVAL_HELP_RECORDS; i++)
  {
    if (revival_help_records[i].device_name.length() == 0)
    {
      target_index = i;
      break;
    }

    if ((long)(revival_help_records[i].expires_at - revival_help_records[target_index].expires_at) < 0)
    {
      target_index = i;
    }
  }

  revival_help_records[target_index].device_name = device_name;
  revival_help_records[target_index].expires_at = now + ttl_ms;
  return true;
}

/**
 * @brief IR 수신
 */
void IrReceive()
{
  if (irrecv.decode(&results))
  {
    if (sur_connect_pending)
    {
      irrecv.resume();
      return;
    }

    // NEC 32bit 프레임만 수용한다. 다른 프로토콜/노이즈/깨진 캡처가 우연히
    // 보수(complement) 체크를 통과해 G208P208 같은 쓰레기값이 나오는 것을 막는다.
    if (results.decode_type != NEC || results.bits != kNECBits)
    {
      irrecv.resume();
      return;
    }

    ir_decode_data = IrDecoding((uint32_t)results.value);
    if ((!ir_receive_error) && (ir_decode_data != "error"))
    {
      DebugPrint("IR DATA : ");
      DebugPrintln(ir_decode_data);
      has2wifi.Receive(ir_decode_data);
      const char *tag_device_name = tag["device_name"].as<const char *>();
      const char *my_device_name = my["device_name"].as<const char *>();
      if (!TextEquals(tag_device_name, my_device_name))
      {
        const char *my_role = my["role"].as<const char *>();
        const char *tag_role = tag["role"].as<const char *>();
        const char *tag_device_state = tag["device_state"].as<const char *>();
        const char *my_device_state = my["device_state"].as<const char *>();

        if (IsTaggerRole(tag_role) && !hacking)
        {
          if (IsPlayerRole(my_role) && TextEquals(tag_device_state, "activate"))
          {
            hack_count++;
            if (hack_count >= HACK_THRESHOLD)
            {
              hack_count = 0;
              hacking = true;
              if (IsFinalLifeTaken())
              {
                StartPendingRevivalVibration();
              }
              has2wifi.Situation(ir_decode_data, "taken");
            }
          }
          else
          {
            hack_count = 0;
          }
        }
        else if (IsRevivalRole(tag_role))
        {
          hack_count = 0;
          if (IsPlayerRole(my_role) && TextEquals(my_device_state, "activate") && !hacking)
          {
            int revival_time = tag["revival_time"].as<int>();
            if (revival_time <= 0)
            {
              revival_time = my["revival_time"].as<int>();
            }
            if (revival_time <= 0)
            {
              revival_time = DEFAULT_REVIVAL_TIME_SEC;
            }

            unsigned long ttl_ms = (unsigned long)revival_time * 1000UL;
            if (!sur_connect_pending && ShouldSendRevivalCooldown(ir_decode_data, ttl_ms))
            {
              StartSurConnect();
              has2wifi.Situation(ir_decode_data, "revival_cooldown");
            }
          }
        }
        else
        {
          hack_count = 0;
        }
      }
    }
  }
  irrecv.resume();
}

/**
 * @brief   수신된 IR데이터를 분석
 *
 * @param ir_data   수신된 IR 데이터
 * @return String   Decoding 성공시 IR 송신자의 device_name을 리턴
 */
String IrDecoding(uint32_t ir_data)
{
  // 0. 수신데이터 나누기
  byte address = (byte)(ir_data >> 24);
  byte address_bar = (byte)((0x00FF) & (ir_data >> 16));
  byte command = (byte)((0x0000FF) & (ir_data >> 8));
  byte command_bar = (byte)((0x000000FF) & (ir_data));

  String ir_decode_data;

  // 1. 수신데이터 오류 판별
  if ((address | address_bar) == 0xFF)
  {
    if ((command | command_bar) == 0xFF && address <= 9 && command <= 9)
    {
      String group_data = (String)(address);
      String player_data = (String)(command);
      ir_decode_data = String("G" + group_data + "P" + player_data);
      ir_receive_error = false;
      return ir_decode_data;
    }
  }

  // 3. 오류가 발생했다면 error를 리턴
  ir_receive_error = true;
  return "error";
}

//******************************************* Battery *******************************************
/**
 * @brief ESP32에 내장되어 있는 배터리 측정 GPIO
 */
void BatteryCheck()
{
  analogRead(34);
  BL.pinRead();
  float battery_volts = BL.getBatteryVolts();

  has2wifi.Send((String)(const char *)my["device_name"], "battery_remaining", String(battery_volts, 2));
}

void MotorInit()
{
  // Linear Motor Init
  pinMode(MOTOR_INA1_PIN, OUTPUT);
  pinMode(MOTOR_INA2_PIN, OUTPUT);
  ledcAttach(MOTOR_PWMA_PIN, MotorFreq, MotorResolution);
  ledcWrite(MOTOR_PWMA_PIN, 0);
}

int Intensity(int intensity)
{
  switch (intensity)
  {
  case 0:
    return 0;
    break;
  case 1:
    return vibration_mode1;
    break;
  case 2:
    return vibration_mode2;
    break;
  case 3:
    return vibration_mode3;
    break;
  case 4:
    return vibration_mode4;
    break;
  case 5:
    return vibration_mode5;
    break;
  default:
    return 0;
    break;
  }
  return 0;
}

void MotorOn(const int *vibration_pattern, int len)
{
  static int repeat = 0;
  if (repeat < len - 1)
  {
    repeat++;
  }
  else
  {
    repeat = 0;
  } // 진동 패턴 인덱스를 한칸씩 증가 시킴
  // Serial.print("repeat : "); Serial.println(repeat);
  // Serial.print("repeat[] : "); Serial.println(vibration_pattern[repeat]);
  ledcWrite(MOTOR_PWMA_PIN, Intensity(vibration_pattern[repeat]));
  delay(20);
  digitalWrite(MOTOR_INA1_PIN, HIGH);
  digitalWrite(MOTOR_INA2_PIN, LOW);
}

void MotorStop()
{
  ledcWrite(MOTOR_PWMA_PIN, 0);
  digitalWrite(MOTOR_INA1_PIN, LOW);
  digitalWrite(MOTOR_INA2_PIN, LOW);
}

//******************************************* Neopixel *******************************************
void UpdateBrightness() {
  int serverBrightness = my["brightness"].as<int>();
  if (serverBrightness <= 0 || serverBrightness > 100) {
    ledBrightness = DEFAULT_BRIGHTNESS;
  } else {
    ledBrightness = map(serverBrightness, 1, 100, 1, 255);
  }
  pixels.setBrightness(ledBrightness);
  pixels.show();
}

void lightColor(int color[])
{
  pixels.fill(pixels.Color(color[0], color[1], color[2]));
  pixels.show();
}

void ota_success_blink()
{
  for (int i = 0; i < 6; i++)
  {
    lightColor(i % 2 == 0 ? red : blue);
    delay(300);
  }
  pixels.clear();
  pixels.show();
}

void tagger_blink()
{
  static bool blink = false;

  if (!blink)
  {
    lightColor(purple);
    blink = true;
  }
  else
  {
    pixels.clear();
    pixels.show();
    blink = false;
  }
}

//******************************************* Beetle ********************************************
/**
 * @brief 게임 내부 가장 가까이에 있는 와이파이 이름을 Beetle로부터 수신받음
 */
bool SendLocationToServer(const String &location)
{
  String device_name = (String)(const char *)my["device_name"];
  if (device_name.length() == 0)
  {
    DebugPrintln("[TTGO] send failed: missing device_name");
    return false;
  }

  HTTPClient location_http;
  String request = String("http://172.30.1.43/has2.php?request=Send&table=device&key=") +
                   device_name + "&column=location&value=" + location;
  location_http.begin(request);
  int http_code = location_http.GET();
  if (http_code == HTTP_CODE_OK)
  {
    location_http.end();
    return true;
  }

  DebugPrintf("[TTGO] send http failed code=%d location=%s\n", http_code, location.c_str());
  location_http.end();
  return false;
}

void QueueLocationSend(const String &location, bool force_now)
{
  if (location == HAS3_UNKNOWN_ROOM)
  {
    if (!location_vibe_muted)
    {
      DebugPrintln("[TTGO] location unknown: mute proximity vibration");
    }
    location_vibe_muted = true;
    pending_location = "";
    last_location_send_fail_at = 0;
    return;
  }

  if (location_vibe_muted)
  {
    DebugPrint("[TTGO] location recovered: unmute proximity vibration=");
    DebugPrintln(location);
    location_vibe_muted = false;
  }

  if (location == last_sent_location && pending_location.length() == 0)
  {
    DebugPrint("[TTGO] skip duplicate location=");
    DebugPrintln(location);
    return;
  }

  unsigned long now = millis();
  if (!force_now && pending_location == location &&
      last_location_send_fail_at != 0 &&
      now - last_location_send_fail_at < 3000)
  {
    DebugPrint("[TTGO] retry pending location=");
    DebugPrint(location);
    DebugPrint(" wait=");
    DebugPrintln((unsigned long)(3000 - (now - last_location_send_fail_at)));
    return;
  }

  DebugPrint("[TTGO] send location=");
  DebugPrintln(location);
  if (SendLocationToServer(location))
  {
    last_sent_location = location;
    pending_location = "";
    last_location_send_fail_at = 0;
    DebugPrint("[TTGO] send success location=");
    DebugPrintln(location);
  }
  else
  {
    pending_location = location;
    last_location_send_fail_at = now;
    DebugPrint("[TTGO] send failed location=");
    DebugPrintln(location);
  }
}

void RetryPendingLocationSend()
{
  if (pending_location.length() == 0 || last_location_send_fail_at == 0)
  {
    return;
  }

  if (millis() - last_location_send_fail_at >= 3000)
  {
    QueueLocationSend(pending_location, true);
  }
}

void ProcessBeetleLine(const char *line, String &loop_location_candidate)
{
  if (line == nullptr || line[0] == '\0')
  {
    return;
  }

  String frame = line;
  frame.trim();
  DebugPrintln(frame);

  if (frame == "reset")
  {
    MySerial1.print((String)(const char *)my["device_state"] + " ");
    return;
  }
  if (frame == "beetle_ota_start")
  {
    FinishBeetleOtaWaitAndRunTtgoOta("beetle_ota_start");
    return;
  }
  if (frame == "beetle_ota_skip")
  {
    FinishBeetleOtaWaitAndRunTtgoOta("beetle_ota_skip");
    return;
  }
  if (frame == "beetle_ota_error")
  {
    FinishBeetleOtaWaitAndRunTtgoOta("beetle_ota_error");
    return;
  }

  if (!frame.startsWith("ROOM:"))
  {
    DebugPrint("[TTGO] invalid frame: ");
    DebugPrintln(frame);
    return;
  }

  String room = frame.substring(5);
  room.trim();
  if (!Has3IsValidRoom(room.c_str()))
  {
    DebugPrint("[TTGO] invalid frame: ");
    DebugPrintln(frame);
    return;
  }

  DebugPrint("[TTGO] received room=");
  DebugPrintln(room);
  loop_location_candidate = room;
}

void BeetleScanWifi()
{
  String loop_location_candidate;

  while (MySerial1.available() > 0)
  {
    char c = (char)MySerial1.read();
    if (c == '\n')
    {
      if (beetle_rx_overflow)
      {
        beetle_rx_overflow = false;
        beetle_rx_len = 0;
        continue;
      }

      if (beetle_rx_len > 0)
      {
        beetle_rx_buffer[beetle_rx_len] = '\0';
        ProcessBeetleLine(beetle_rx_buffer, loop_location_candidate);
        beetle_rx_len = 0;
      }
      continue;
    }

    if (c == '\r')
    {
      continue;
    }

    if (beetle_rx_overflow)
    {
      continue;
    }

    if (beetle_rx_len < BEETLE_RX_BUFFER_SIZE - 1)
    {
      beetle_rx_buffer[beetle_rx_len++] = c;
    }
    else
    {
      DebugPrintln("[TTGO] uart overflow, drop line");
      beetle_rx_overflow = true;
      beetle_rx_len = 0;
    }
  }

  if (loop_location_candidate.length() > 0)
  {
    if (game_state == activate)
    {
      QueueLocationSend(loop_location_candidate, true);
    }
    else
    {
      DebugPrint("[TTGO] skip location while inactive=");
      DebugPrintln(loop_location_candidate);
    }
  }

  RetryPendingLocationSend();
}
