#include "updated_IoTglove.h"

#define NEXTION_TFT_PC_BAUD 921600
#define NEXTION_TFT_DEFAULT_CHUNK_SIZE 4096
#define NEXTION_TFT_PC_RX_BUFFER_SIZE 8192
#define NEXTION_TFT_NEXTION_RX_BUFFER_SIZE 512
#define NEXTION_TFT_COMMAND_TIMEOUT_MS 1000
#define NEXTION_TFT_PC_DATA_TIMEOUT_MS 10000
#define NEXTION_TFT_CONNECT_TIMEOUT_MS 2000
#define NEXTION_TFT_START_ACK_TIMEOUT_MS 5000
#define NEXTION_TFT_CHUNK_ACK_TIMEOUT_MS 10000
#define NEXTION_TFT_READY_TIMEOUT_MS 30000
#define NEXTION_TFT_SESSION_IDLE_TIMEOUT_MS 15000

static uint8_t nextion_tft_chunk_buffer[NEXTION_TFT_DEFAULT_CHUNK_SIZE];
static uint32_t nextion_tft_expected_bytes = 0;
static uint32_t nextion_tft_uploaded_bytes = 0;
static bool nextion_tft_upload_active = false;
static bool nextion_tft_serial_ready = false;
static bool nextion_tft_session_done = false;
static uint32_t nextion_tft_pc_serial_baud = 0;

String NextionTftByteToHex(uint8_t value)
{
  String text = String(value, HEX);
  text.toUpperCase();
  if (value < 0x10)
  {
    text = "0" + text;
  }
  return text;
}

void NextionTftAppendTraceByte(String *trace, uint8_t value)
{
  if (trace == nullptr || trace->length() >= 180)
  {
    return;
  }

  if (trace->length() > 0)
  {
    *trace += " ";
  }

  *trace += NextionTftByteToHex(value);
}

void NextionTftSendTerminator()
{
  MySerial2.write(0xFF);
  MySerial2.write(0xFF);
  MySerial2.write(0xFF);
}

void NextionTftSendCommand(const String &command)
{
  MySerial2.print(command);
  NextionTftSendTerminator();
  MySerial2.flush();
}

uint32_t NextionTftDelayForBaud(uint32_t baud)
{
  if (baud == 0)
  {
    return 150;
  }

  return (1000000UL / baud) + 30UL;
}

void NextionTftDrainDisplay()
{
  while (MySerial2.available() > 0)
  {
    MySerial2.read();
  }
}

void NextionTftDrainDisplayFor(uint32_t duration_ms)
{
  uint32_t start_ms = millis();

  while (millis() - start_ms < duration_ms)
  {
    NextionTftDrainDisplay();
    delay(1);
  }
}

void NextionTftExitReparseMode(uint32_t baud)
{
  NextionTftSendCommand("DRAKJHSUYDGBNCJHGJKSHBDN");
  delay(NextionTftDelayForBaud(baud));
  NextionTftSendCommand("recmod=0");
  delay(NextionTftDelayForBaud(baud));
  NextionTftSendCommand("recmod=0");
  delay(NextionTftDelayForBaud(baud));
  NextionTftDrainDisplayFor(300);
}

bool NextionTftReadPcLine(String &line, uint32_t timeout_ms)
{
  line = "";
  uint32_t start_ms = millis();

  while (millis() - start_ms < timeout_ms)
  {
    while (Serial.available() > 0)
    {
      char c = (char)Serial.read();

      if (c == '\n')
      {
        line.trim();
        return true;
      }

      if (c != '\r' && line.length() < 160)
      {
        line += c;
      }
    }

    delay(1);
  }

  return false;
}

bool NextionTftReadPcBytes(uint8_t *buffer, size_t length, uint32_t timeout_ms)
{
  size_t received = 0;
  uint32_t last_byte_ms = millis();

  while (received < length)
  {
    while (Serial.available() > 0 && received < length)
    {
      buffer[received++] = (uint8_t)Serial.read();
      last_byte_ms = millis();
    }

    if (millis() - last_byte_ms > timeout_ms)
    {
      return false;
    }

    delay(1);
  }

  return true;
}

bool NextionTftWaitForByte(uint8_t expected, uint32_t timeout_ms, String *trace = nullptr)
{
  if (trace != nullptr)
  {
    *trace = "";
  }

  uint32_t start_ms = millis();

  while (millis() - start_ms < timeout_ms)
  {
    while (MySerial2.available() > 0)
    {
      uint8_t value = (uint8_t)MySerial2.read();
      NextionTftAppendTraceByte(trace, value);
      if (value == expected)
      {
        return true;
      }
    }

    delay(1);
  }

  return false;
}

uint8_t NextionTftWaitForUploadAck(uint32_t timeout_ms, String *trace, uint32_t &next_offset)
{
  next_offset = 0;
  if (trace != nullptr)
  {
    *trace = "";
  }

  uint32_t start_ms = millis();

  while (millis() - start_ms < timeout_ms)
  {
    while (MySerial2.available() > 0)
    {
      uint8_t value = (uint8_t)MySerial2.read();
      NextionTftAppendTraceByte(trace, value);

      if (value == 0x05)
      {
        return 0x05;
      }

      if (value == 0x08)
      {
        uint8_t offset_bytes[4] = {0, 0, 0, 0};
        uint8_t received = 0;
        uint32_t offset_start_ms = millis();

        while (received < 4 && millis() - offset_start_ms < timeout_ms)
        {
          while (MySerial2.available() > 0 && received < 4)
          {
            offset_bytes[received] = (uint8_t)MySerial2.read();
            NextionTftAppendTraceByte(trace, offset_bytes[received]);
            received++;
          }

          delay(1);
        }

        if (received == 4)
        {
          next_offset = ((uint32_t)offset_bytes[0]) |
                        ((uint32_t)offset_bytes[1] << 8) |
                        ((uint32_t)offset_bytes[2] << 16) |
                        ((uint32_t)offset_bytes[3] << 24);
        }

        return 0x08;
      }
    }

    delay(1);
  }

  return 0;
}

bool NextionTftWaitForConnect(String &response)
{
  response = "";
  bool saw_comok = false;
  int terminator_count = 0;
  uint32_t start_ms = millis();

  while (millis() - start_ms < NEXTION_TFT_CONNECT_TIMEOUT_MS)
  {
    while (MySerial2.available() > 0)
    {
      uint8_t value = (uint8_t)MySerial2.read();

      if (value == 0xFF)
      {
        terminator_count++;
        if (saw_comok && terminator_count >= 3)
        {
          return true;
        }
        continue;
      }

      terminator_count = 0;

      if (value >= 32 && value <= 126 && response.length() < 140)
      {
        response += (char)value;
      }

      if (response.indexOf("comok") >= 0)
      {
        saw_comok = true;
      }
    }

    delay(1);
  }

  return saw_comok;
}

bool NextionTftConnectDisplay(uint32_t nextion_baud, String &response)
{
  MySerial2.begin(nextion_baud, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  delay(50);
  NextionTftDrainDisplay();

  NextionTftSendCommand("");
  delay(NextionTftDelayForBaud(nextion_baud));

  NextionTftExitReparseMode(nextion_baud);

  NextionTftSendCommand("connect");
  delay(NextionTftDelayForBaud(nextion_baud));

  bool connected = NextionTftWaitForConnect(response);
  NextionTftDrainDisplayFor(300);

  return connected;
}

bool NextionTftParseBeginCommand(const String &line, uint32_t &file_size, uint32_t &nextion_baud, uint32_t &upload_baud, bool &verify_connect)
{
  int first_space = line.indexOf(' ');
  int second_space = line.indexOf(' ', first_space + 1);
  int third_space = line.indexOf(' ', second_space + 1);
  int fourth_space = line.indexOf(' ', third_space + 1);

  if (first_space < 0 || second_space < 0 || third_space < 0 || fourth_space < 0)
  {
    return false;
  }

  file_size = line.substring(first_space + 1, second_space).toInt();
  nextion_baud = line.substring(second_space + 1, third_space).toInt();
  upload_baud = line.substring(third_space + 1, fourth_space).toInt();
  verify_connect = line.substring(fourth_space + 1).toInt() != 0;

  return file_size > 0 && nextion_baud > 0 && upload_baud > 0;
}

void NextionTftBeginUpload(const String &line)
{
  uint32_t file_size = 0;
  uint32_t nextion_baud = 0;
  uint32_t upload_baud = 0;
  bool verify_connect = true;

  if (!NextionTftParseBeginCommand(line, file_size, nextion_baud, upload_baud, verify_connect))
  {
    Serial.println("ERR bad_begin");
    nextion_tft_session_done = true;
    return;
  }

  String connect_response;
  if (verify_connect)
  {
    if (!NextionTftConnectDisplay(nextion_baud, connect_response))
    {
      Serial.println("ERR connect_timeout");
      nextion_tft_session_done = true;
      return;
    }

    Serial.print("INFO connect ");
    Serial.println(connect_response);
  }
  else
  {
    MySerial2.begin(nextion_baud, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
    delay(50);
    NextionTftDrainDisplay();
  }

  NextionTftExitReparseMode(nextion_baud);

  NextionTftSendCommand("sleep=0");
  NextionTftSendCommand("dim=100");
  delay(250);
  NextionTftDrainDisplayFor(300);

  String upload_command = "whmi-wris " + String(file_size) + "," + String(upload_baud) + ",1";
  NextionTftSendCommand(upload_command);

  String ack_trace;
  if (upload_baud != nextion_baud)
  {
    MySerial2.flush();
    MySerial2.end();
    delay(20);
    MySerial2.begin(upload_baud, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  }

  bool upload_ready = NextionTftWaitForByte(0x05, NEXTION_TFT_START_ACK_TIMEOUT_MS, &ack_trace);

  if (!upload_ready)
  {
    Serial.print("ERR whmi_ack_timeout trace=");
    Serial.println(ack_trace.length() > 0 ? ack_trace : "none");
    nextion_tft_session_done = true;
    return;
  }

  nextion_tft_expected_bytes = file_size;
  nextion_tft_uploaded_bytes = 0;
  nextion_tft_upload_active = true;

  Serial.print("BEGIN_OK ");
  Serial.println(NEXTION_TFT_DEFAULT_CHUNK_SIZE);
}

bool NextionTftParseResumeCommand(const String &line, uint32_t &file_size, uint32_t &upload_baud)
{
  int first_space = line.indexOf(' ');
  int second_space = line.indexOf(' ', first_space + 1);

  if (first_space < 0 || second_space < 0)
  {
    return false;
  }

  file_size = line.substring(first_space + 1, second_space).toInt();
  upload_baud = line.substring(second_space + 1).toInt();

  return file_size > 0 && upload_baud > 0;
}

void NextionTftResumeUpload(const String &line)
{
  uint32_t file_size = 0;
  uint32_t upload_baud = 0;

  if (!NextionTftParseResumeCommand(line, file_size, upload_baud))
  {
    Serial.println("ERR bad_resume");
    nextion_tft_session_done = true;
    return;
  }

  MySerial2.begin(upload_baud, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  delay(50);

  nextion_tft_expected_bytes = file_size;
  nextion_tft_uploaded_bytes = 0;
  nextion_tft_upload_active = true;

  Serial.print("BEGIN_OK ");
  Serial.println(NEXTION_TFT_DEFAULT_CHUNK_SIZE);
}

bool NextionTftParseDataCommand(const String &line, uint32_t &length)
{
  if (!line.startsWith("DATA "))
  {
    return false;
  }

  length = line.substring(5).toInt();
  return length > 0 && length <= NEXTION_TFT_DEFAULT_CHUNK_SIZE;
}

void NextionTftReceiveChunk(const String &line)
{
  if (!nextion_tft_upload_active)
  {
    Serial.println("ERR upload_not_active");
    nextion_tft_session_done = true;
    return;
  }

  uint32_t length = 0;
  if (!NextionTftParseDataCommand(line, length))
  {
    Serial.println("ERR bad_data");
    nextion_tft_session_done = true;
    return;
  }

  if (nextion_tft_uploaded_bytes + length > nextion_tft_expected_bytes)
  {
    Serial.println("ERR data_too_large");
    nextion_tft_upload_active = false;
    nextion_tft_session_done = true;
    return;
  }

  if (!NextionTftReadPcBytes(nextion_tft_chunk_buffer, length, NEXTION_TFT_PC_DATA_TIMEOUT_MS))
  {
    Serial.println("ERR pc_data_timeout");
    nextion_tft_upload_active = false;
    nextion_tft_session_done = true;
    return;
  }

  MySerial2.write(nextion_tft_chunk_buffer, length);
  MySerial2.flush();

  String chunk_trace;
  uint32_t next_offset = 0;
  uint8_t ack = NextionTftWaitForUploadAck(NEXTION_TFT_CHUNK_ACK_TIMEOUT_MS, &chunk_trace, next_offset);

  if (ack == 0)
  {
    Serial.print("ERR chunk_ack_timeout trace=");
    Serial.println(chunk_trace.length() > 0 ? chunk_trace : "none");
    nextion_tft_upload_active = false;
    nextion_tft_session_done = true;
    return;
  }

  if (ack == 0x08 && next_offset > 0 && next_offset <= nextion_tft_expected_bytes)
  {
    nextion_tft_uploaded_bytes = next_offset;
  }
  else
  {
    nextion_tft_uploaded_bytes += length;
  }

  Serial.print("DATA_OK ");
  Serial.println(nextion_tft_uploaded_bytes);

  if (nextion_tft_uploaded_bytes >= nextion_tft_expected_bytes)
  {
    nextion_tft_upload_active = false;

    if (NextionTftWaitForByte(0x88, NEXTION_TFT_READY_TIMEOUT_MS))
    {
      Serial.println("DONE ready");
    }
    else
    {
      Serial.println("DONE no_ready_notice");
    }

    nextion_tft_session_done = true;
  }
}

void NextionTftSetPcSerialBaud(uint32_t baud)
{
  if (nextion_tft_serial_ready && nextion_tft_pc_serial_baud == baud)
  {
    return;
  }

  if (nextion_tft_serial_ready)
  {
    Serial.flush();
    Serial.end();
    delay(20);
  }

  Serial.setRxBufferSize(NEXTION_TFT_PC_RX_BUFFER_SIZE);
  Serial.begin(baud);
  Serial.setTimeout(50);
  nextion_tft_serial_ready = true;
  nextion_tft_pc_serial_baud = baud;
}

void NextionTftUploadInit()
{
  MySerial2.setRxBufferSize(NEXTION_TFT_NEXTION_RX_BUFFER_SIZE);
  NextionTftSetPcSerialBaud(NEXTION_TFT_PC_BAUD);
}

void NextionTftUploadUseBootSerial()
{
  NextionTftSetPcSerialBaud(BOOT_SERIAL_BAUD);
}

void NextionTftUploadRestoreDisplaySerial()
{
  MySerial2.flush();
  MySerial2.end();
  delay(20);
  MySerial2.begin(9600, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  NextionTftDrainDisplay();
}

void NextionTftHandleCommand(const String &line)
{
  if (line == "NXUP_HELLO")
  {
    Serial.println("NXUP_READY 1");
    return;
  }

  if (line.startsWith("PROBE "))
  {
    uint32_t baud = line.substring(6).toInt();
    String response;

    if (baud == 0)
    {
      Serial.println("ERR bad_probe");
      return;
    }

    if (NextionTftConnectDisplay(baud, response))
    {
      Serial.print("PROBE_OK ");
      Serial.print(baud);
      Serial.print(" ");
      Serial.println(response);
    }
    else
    {
      Serial.print("ERR probe_timeout ");
      Serial.println(baud);
    }
    return;
  }

  if (line.startsWith("SEND "))
  {
    int first_space = line.indexOf(' ');
    int second_space = line.indexOf(' ', first_space + 1);

    if (first_space < 0 || second_space < 0)
    {
      Serial.println("ERR bad_send");
      return;
    }

    uint32_t baud = line.substring(first_space + 1, second_space).toInt();
    String command = line.substring(second_space + 1);

    if (baud == 0 || command.length() == 0)
    {
      Serial.println("ERR bad_send");
      return;
    }

    MySerial2.begin(baud, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
    delay(50);
    NextionTftDrainDisplay();
    NextionTftSendCommand(command);
    Serial.println("SEND_OK");
    return;
  }

  if (line.startsWith("BEGIN "))
  {
    NextionTftBeginUpload(line);
    return;
  }

  if (line.startsWith("RESUME "))
  {
    NextionTftResumeUpload(line);
    return;
  }

  if (line.startsWith("DATA "))
  {
    NextionTftReceiveChunk(line);
    return;
  }

  if (line == "ABORT")
  {
    nextion_tft_upload_active = false;
    nextion_tft_session_done = true;
    Serial.println("ABORT_OK");
    return;
  }

  Serial.println("ERR unknown_command");
}

void NextionTftRunSession()
{
  nextion_tft_session_done = false;
  uint32_t last_command_ms = millis();

  while (!nextion_tft_session_done)
  {
    String line;
    if (!NextionTftReadPcLine(line, NEXTION_TFT_COMMAND_TIMEOUT_MS))
    {
      if (!nextion_tft_upload_active && millis() - last_command_ms > NEXTION_TFT_SESSION_IDLE_TIMEOUT_MS)
      {
        break;
      }
      continue;
    }

    last_command_ms = millis();
    NextionTftHandleCommand(line);
  }
}

bool NextionTftStartFromLine(const String &line)
{
  if (line != "NXUP_HELLO")
  {
    return false;
  }

  nextion_tft_upload_active = false;
  nextion_tft_expected_bytes = 0;
  nextion_tft_uploaded_bytes = 0;
  NextionTftSetPcSerialBaud(NEXTION_TFT_PC_BAUD);
  Serial.println("NXUP_READY 1");
  NextionTftRunSession();
  NextionTftUploadRestoreDisplaySerial();
  NextionTftUploadUseBootSerial();
  return true;
}

bool NextionTftUploadStartupWindow(unsigned long window_ms)
{
  uint32_t start_ms = millis();

  while (millis() - start_ms < window_ms)
  {
    if (Serial.available() <= 0)
    {
      delay(1);
      continue;
    }

    String line;
    if (NextionTftReadPcLine(line, 100))
    {
      return NextionTftStartFromLine(line);
    }
  }

  return false;
}

bool NextionTftUploadPoll()
{
  if (Serial.available() <= 0)
  {
    return false;
  }

  String line;
  if (!NextionTftReadPcLine(line, 100))
  {
    return false;
  }

  return NextionTftStartFromLine(line);
}
