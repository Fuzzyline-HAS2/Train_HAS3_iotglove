#include "wifi_location.h"

static const char *RoomNameFromIndex(int8_t room_index)
{
  if (room_index < 0 || room_index >= HAS3_ROOM_COUNT)
  {
    return HAS3_UNKNOWN_ROOM;
  }
  return HAS3_ROOMS[room_index];
}

static void OnBleScanComplete(BLEScanResults results)
{
  (void)results;
  ble_scan_segment_done = true;
}

class Has3AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertised_device)
  {
    if (!advertised_device.haveName())
    {
      return;
    }

    String local_name = advertised_device.getName().c_str();
    local_name.trim();
    char device_name[HAS3_DEVICE_NAME_BUFFER_SIZE] = {0};
    if (!Has3ParseLocalName(local_name.c_str(), device_name, sizeof(device_name)))
    {
      return;
    }

    if (!Has3PayloadHasCompleteLocalName(
            advertised_device.getPayload(),
            advertised_device.getPayloadLength(),
            local_name.c_str()))
    {
      return;
    }

    const char *room = Has3RoomFromDeviceName(device_name);
    if (room == nullptr)
    {
      return;
    }

    AddBleSample(device_name, room, advertised_device.getRSSI(), millis());
  }
};

void InitBleScanner()
{
  BLEDevice::init("");
  ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new Has3AdvertisedDeviceCallbacks(), true);
  ble_scan->setActiveScan(false);
  ble_scan->setInterval(BLE_SCAN_INTERVAL);
  ble_scan->setWindow(BLE_SCAN_WINDOW);
}

void ResetBleLocationState()
{
  ble_sample_count = 0;
  for (int i = 0; i < BLE_MAX_SAMPLES; i++)
  {
    ble_samples[i].device_name[0] = '\0';
    ble_samples[i].room_index = 0;
    ble_samples[i].rssi = -127;
    ble_samples[i].timestamp_ms = 0;
  }

  for (int i = 0; i < BLE_MAX_DEVICES; i++)
  {
    ble_devices[i].device_name[0] = '\0';
    ble_devices[i].room_index = 0;
    ble_devices[i].has_ema = false;
    ble_devices[i].ema = -127.0f;
    ble_devices[i].last_seen_ms = 0;
  }

  stable_room_index = -1;
  candidate_room_index = -1;
  candidate_started_ms = 0;
  last_room_sent_ms = 0;
  last_valid_has3_ms = 0;
  ble_location_started_ms = millis();
  last_ble_eval_ms = 0;
  ble_scan_segment_done = false;
}

static int FindDeviceState(const char *device_name)
{
  for (int i = 0; i < BLE_MAX_DEVICES; i++)
  {
    if (ble_devices[i].device_name[0] != '\0' && strcmp(ble_devices[i].device_name, device_name) == 0)
    {
      return i;
    }
  }
  return -1;
}

static int EnsureDeviceState(const char *device_name, uint8_t room_index)
{
  int index = FindDeviceState(device_name);
  if (index >= 0)
  {
    ble_devices[index].room_index = room_index;
    return index;
  }

  for (int i = 0; i < BLE_MAX_DEVICES; i++)
  {
    if (ble_devices[i].device_name[0] == '\0')
    {
      strncpy(ble_devices[i].device_name, device_name, sizeof(ble_devices[i].device_name));
      ble_devices[i].device_name[sizeof(ble_devices[i].device_name) - 1] = '\0';
      ble_devices[i].room_index = room_index;
      ble_devices[i].has_ema = false;
      ble_devices[i].ema = -127.0f;
      ble_devices[i].last_seen_ms = 0;
      return i;
    }
  }

  return -1;
}

void AddBleSample(const char *device_name, const char *room, int rssi, unsigned long timestamp_ms)
{
  int room_index = Has3RoomIndex(room);
  if (!Has3IsValidDeviceName(device_name) || room_index < 0)
  {
    return;
  }

  int device_index = EnsureDeviceState(device_name, (uint8_t)room_index);
  if (device_index < 0)
  {
    return;
  }

  ble_devices[device_index].last_seen_ms = timestamp_ms;
  last_valid_has3_ms = timestamp_ms;

  BleSample *sample = nullptr;
  if (ble_sample_count < BLE_MAX_SAMPLES)
  {
    sample = &ble_samples[ble_sample_count++];
  }
  else
  {
    uint8_t oldest = 0;
    for (uint8_t i = 1; i < BLE_MAX_SAMPLES; i++)
    {
      if (ble_samples[i].timestamp_ms < ble_samples[oldest].timestamp_ms)
      {
        oldest = i;
      }
    }
    sample = &ble_samples[oldest];
  }

  strncpy(sample->device_name, device_name, sizeof(sample->device_name));
  sample->device_name[sizeof(sample->device_name) - 1] = '\0';
  sample->room_index = (uint8_t)room_index;
  sample->rssi = (int8_t)constrain(rssi, -127, 20);
  sample->timestamp_ms = timestamp_ms;
}

static void PruneOldSamples(unsigned long now)
{
  uint8_t write_index = 0;
  for (uint8_t read_index = 0; read_index < ble_sample_count; read_index++)
  {
    if (now - ble_samples[read_index].timestamp_ms <= BLE_SAMPLE_WINDOW_MS)
    {
      if (write_index != read_index)
      {
        ble_samples[write_index] = ble_samples[read_index];
      }
      write_index++;
    }
  }
  ble_sample_count = write_index;
}

static void PruneOldDevices(unsigned long now)
{
  for (int i = 0; i < BLE_MAX_DEVICES; i++)
  {
    if (ble_devices[i].device_name[0] != '\0' &&
        ble_devices[i].last_seen_ms != 0 &&
        now - ble_devices[i].last_seen_ms > BLE_DEVICE_EMA_KEEP_MS)
    {
      ble_devices[i].device_name[0] = '\0';
      ble_devices[i].has_ema = false;
      ble_devices[i].ema = -127.0f;
    }
  }
}

static int MedianOf(int values[], int count)
{
  for (int i = 0; i < count - 1; i++)
  {
    for (int j = i + 1; j < count; j++)
    {
      if (values[j] < values[i])
      {
        int temp = values[i];
        values[i] = values[j];
        values[j] = temp;
      }
    }
  }

  if (count % 2 == 1)
  {
    return values[count / 2];
  }
  return (values[(count / 2) - 1] + values[count / 2]) / 2;
}

static void UpdateDeviceEma(unsigned long now, bool active_devices[])
{
  for (int device_index = 0; device_index < BLE_MAX_DEVICES; device_index++)
  {
    if (ble_devices[device_index].device_name[0] == '\0')
    {
      continue;
    }

    int values[BLE_MAX_SAMPLES];
    int count = 0;
    for (uint8_t sample_index = 0; sample_index < ble_sample_count; sample_index++)
    {
      if (strcmp(ble_samples[sample_index].device_name, ble_devices[device_index].device_name) == 0)
      {
        values[count++] = ble_samples[sample_index].rssi;
      }
    }

    active_devices[device_index] = count >= BLE_MIN_DEVICE_SAMPLES;
    if (!active_devices[device_index])
    {
      continue;
    }

    int median = MedianOf(values, count);
    if (ble_devices[device_index].has_ema)
    {
      ble_devices[device_index].ema = ble_devices[device_index].ema * 0.5f + median * 0.5f;
    }
    else
    {
      ble_devices[device_index].ema = median;
      ble_devices[device_index].has_ema = true;
    }
    ble_devices[device_index].last_seen_ms = now;
  }
}

static void BuildRoomScores(const bool active_devices[], BleRoomScore scores[])
{
  for (int room = 0; room < HAS3_ROOM_COUNT; room++)
  {
    scores[room].active = false;
    scores[room].score = -127.0f;
    scores[room].devices = 0;
  }

  for (int room = 0; room < HAS3_ROOM_COUNT; room++)
  {
    float top1 = -127.0f;
    float top2 = -127.0f;
    uint8_t count = 0;

    for (int device_index = 0; device_index < BLE_MAX_DEVICES; device_index++)
    {
      if (!active_devices[device_index] ||
          ble_devices[device_index].device_name[0] == '\0' ||
          ble_devices[device_index].room_index != room ||
          !ble_devices[device_index].has_ema)
      {
        continue;
      }

      float value = ble_devices[device_index].ema;
      count++;
      if (value > top1)
      {
        top2 = top1;
        top1 = value;
      }
      else if (value > top2)
      {
        top2 = value;
      }
    }

    if (count == 0)
    {
      continue;
    }

    scores[room].active = true;
    scores[room].devices = count;
    scores[room].score = count >= 2 ? (top1 + top2) / 2.0f : top1;
  }
}

static int8_t BestRoom(const BleRoomScore scores[])
{
  int8_t best_room = -1;
  float best_score = -127.0f;
  for (int room = 0; room < HAS3_ROOM_COUNT; room++)
  {
    if (scores[room].active && (best_room < 0 || scores[room].score > best_score))
    {
      best_room = room;
      best_score = scores[room].score;
    }
  }
  return best_room;
}

static void SendRoom(const char *room)
{
  Serial.print("[BEETLE] send ROOM:");
  Serial.println(room);
  MySerial1.print("ROOM:");
  MySerial1.print(room);
  MySerial1.print("\n");
  last_room_sent_ms = millis();
}

static void SetStableRoom(int8_t next_room, const char *reason)
{
  if (next_room < 0 || next_room >= HAS3_ROOM_COUNT)
  {
    return;
  }

  if (stable_room_index != next_room)
  {
    Serial.print("[BEETLE] stableRoom changed ");
    Serial.print(RoomNameFromIndex(stable_room_index));
    Serial.print(" -> ");
    Serial.println(RoomNameFromIndex(next_room));
    stable_room_index = next_room;
    candidate_room_index = -1;
    candidate_started_ms = 0;
    SendRoom(RoomNameFromIndex(stable_room_index));
    if (reason != nullptr)
    {
      Serial.print("[BEETLE] reason=");
      Serial.println(reason);
    }
  }
}

static void EnterUnknown(const char *reason)
{
  if (stable_room_index != -2)
  {
    Serial.print("[BEETLE] unknown entered reason=");
    Serial.println(reason);
    stable_room_index = -2;
    candidate_room_index = -1;
    candidate_started_ms = 0;
    SendRoom(HAS3_UNKNOWN_ROOM);
  }
}

static void LogScores(const BleRoomScore scores[])
{
  Serial.print("[BEETLE] scores");
  for (int room = 0; room < HAS3_ROOM_COUNT; room++)
  {
    if (!scores[room].active)
    {
      continue;
    }
    Serial.print(" ");
    Serial.print(HAS3_ROOMS[room]);
    Serial.print("=");
    Serial.print(scores[room].score, 1);
  }
  Serial.println();
}

static void UpdateStableRoom(unsigned long now, const BleRoomScore scores[])
{
  int8_t best_room = BestRoom(scores);

  if (best_room >= 0 && stable_room_index == -2)
  {
    Serial.print("[BEETLE] unknown recovered by ");
    Serial.println(RoomNameFromIndex(best_room));
    SetStableRoom(best_room, "unknown recovered");
    return;
  }

  if (best_room < 0)
  {
    candidate_room_index = -1;
    candidate_started_ms = 0;
    if ((last_valid_has3_ms == 0 && now - ble_location_started_ms >= BLE_UNKNOWN_TIMEOUT_MS) ||
        (last_valid_has3_ms != 0 && now - last_valid_has3_ms >= BLE_UNKNOWN_TIMEOUT_MS))
    {
      EnterUnknown("no valid HAS3 beacon for 5s");
    }
    return;
  }

  if (stable_room_index < 0 || stable_room_index >= HAS3_ROOM_COUNT)
  {
    if (candidate_room_index != best_room)
    {
      candidate_room_index = best_room;
      candidate_started_ms = now;
    }
    else if (now - candidate_started_ms >= BLE_SWITCH_HOLD_MS)
    {
      SetStableRoom(best_room, "initial stable");
    }
    return;
  }

  if (best_room == stable_room_index)
  {
    candidate_room_index = -1;
    candidate_started_ms = 0;
    return;
  }

  bool stable_has_score = scores[stable_room_index].active;
  bool margin_ok = !stable_has_score || scores[best_room].score >= scores[stable_room_index].score + 5.0f;
  if (!margin_ok)
  {
    candidate_room_index = -1;
    candidate_started_ms = 0;
    return;
  }

  if (candidate_room_index != best_room)
  {
    candidate_room_index = best_room;
    candidate_started_ms = now;
    Serial.print("[BEETLE] candidateRoom=");
    Serial.print(RoomNameFromIndex(best_room));
    Serial.print(" reason=+");
    Serial.print(stable_has_score ? scores[best_room].score - scores[stable_room_index].score : 0.0f, 1);
    Serial.print("dB over ");
    Serial.println(RoomNameFromIndex(stable_room_index));
    return;
  }

  if (now - candidate_started_ms >= BLE_SWITCH_HOLD_MS)
  {
    SetStableRoom(best_room, "candidate held 1.2s");
  }
}

static void EvaluateBleLocation(unsigned long now)
{
  PruneOldSamples(now);
  PruneOldDevices(now);

  bool active_devices[BLE_MAX_DEVICES] = {false};
  BleRoomScore scores[HAS3_ROOM_COUNT];
  UpdateDeviceEma(now, active_devices);
  BuildRoomScores(active_devices, scores);
  LogScores(scores);
  UpdateStableRoom(now, scores);

  if (stable_room_index == -2 && now - last_room_sent_ms >= BLE_ROOM_REPEAT_MS)
  {
    SendRoom(HAS3_UNKNOWN_ROOM);
  }
  else if (stable_room_index >= 0 && now - last_room_sent_ms >= BLE_ROOM_REPEAT_MS)
  {
    SendRoom(RoomNameFromIndex(stable_room_index));
  }
}

static void EnsureBleScanRunning()
{
  if (ble_scan == nullptr)
  {
    return;
  }

  if (ble_scan_segment_done)
  {
    ble_scan->clearResults();
    ble_scan_segment_done = false;
  }

  if (!ble_scan->isScanning())
  {
    if (!ble_scan->start(BLE_SCAN_SECONDS, OnBleScanComplete, true))
    {
      Serial.println("[BEETLE] BLE scan start failed");
    }
  }
}

void ScanBleLocation()
{
  if (game_state != activate || ble_scan == nullptr)
  {
    if (ble_scan != nullptr && ble_scan->isScanning())
    {
      ble_scan->stop();
    }
    digitalWrite(BEETLE_LED_PIN, LOW);
    return;
  }

  digitalWrite(BEETLE_LED_PIN, HIGH);
  EnsureBleScanRunning();

  unsigned long now = millis();
  if (last_ble_eval_ms == 0 || now - last_ble_eval_ms >= BLE_EVAL_INTERVAL_MS)
  {
    last_ble_eval_ms = now;
    EvaluateBleLocation(now);
  }
}
