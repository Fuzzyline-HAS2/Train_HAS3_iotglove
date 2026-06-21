#include "wifi_location.h"

class Has2AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertised_device)
  {
    if (!advertised_device.haveName())
    {
      return;
    }

    String id = advertised_device.getName().c_str();
    id.trim();
    AddBleSample(id, advertised_device.getRSSI());
  }
};

void InitBleScanner()
{
  BLEDevice::init("");
  ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new Has2AdvertisedDeviceCallbacks());
  ble_scan->setActiveScan(true);
  ble_scan->setInterval(BLE_SCAN_INTERVAL);
  ble_scan->setWindow(BLE_SCAN_WINDOW);
}

void ResetBleCandidates()
{
  ble_candidate_count = 0;
  for (int i = 0; i < BLE_MAX_CANDIDATES; i++)
  {
    ble_candidates[i].id = "";
    ble_candidates[i].rssi_sum = 0;
    ble_candidates[i].samples = 0;
  }
}

void AddBleSample(const String &id, int rssi)
{
  if (!id.startsWith(BLE_NAME_PREFIX))
  {
    return;
  }

  for (int i = 0; i < ble_candidate_count; i++)
  {
    if (ble_candidates[i].id == id)
    {
      ble_candidates[i].rssi_sum += rssi;
      ble_candidates[i].samples++;
      return;
    }
  }

  if (ble_candidate_count >= BLE_MAX_CANDIDATES)
  {
    return;
  }

  ble_candidates[ble_candidate_count].id = id;
  ble_candidates[ble_candidate_count].rssi_sum = rssi;
  ble_candidates[ble_candidate_count].samples = 1;
  ble_candidate_count++;
}

String BestBleCandidate()
{
  String best_id;
  int best_average = -1000;

  for (int i = 0; i < ble_candidate_count; i++)
  {
    if (ble_candidates[i].samples <= 0)
    {
      continue;
    }

    int average = ble_candidates[i].rssi_sum / ble_candidates[i].samples;
    Serial.print("[BLE] ");
    Serial.print(ble_candidates[i].id);
    Serial.print(" avg ");
    Serial.print(average);
    Serial.print(" samples ");
    Serial.println(ble_candidates[i].samples);

    if (average > best_average)
    {
      best_average = average;
      best_id = ble_candidates[i].id;
    }
  }

  return best_id;
}

void SendStableLocation(const String &candidate_id)
{
  if (candidate_id.length() == 0)
  {
    stable_candidate_count = 0;
    stable_candidate_id = "";
    return;
  }

  if (stable_candidate_id == candidate_id)
  {
    stable_candidate_count++;
  }
  else
  {
    stable_candidate_id = candidate_id;
    stable_candidate_count = 1;
  }

  if (stable_candidate_count >= BLE_STABLE_REQUIRED && last_sent_id != candidate_id)
  {
    last_sent_id = candidate_id;
    Serial.print("[BLE] send ");
    Serial.println(candidate_id);
    MySerial1.print(candidate_id + " ");
  }
}

void ScanBleLocation()
{
  if (game_state != activate || ble_scan == nullptr)
  {
    return;
  }

  digitalWrite(BEETLE_LED_PIN, HIGH);
  ResetBleCandidates();
  ble_scan->start(BLE_SCAN_SECONDS, false);
  String best_id = BestBleCandidate();
  ble_scan->clearResults();
  SendStableLocation(best_id);
  digitalWrite(BEETLE_LED_PIN, LOW);
}
