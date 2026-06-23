#include "Train_HAS3_iotglove.h"

/**
 * @brief 약신호(-80dBm 이하) 링크 안정화 설정. WiFi.begin 전에 호출.
 *        - 모뎀 슬립 off (약신호에서 지연/누락 감소)
 *        - 최대 TX 파워 20dBm
 *        - 끊기면 자동 재연결
 *  ※ -80dBm 이하에서도 연결을 유지하는 핵심은 AP의
 *    Minimum RSSI 차단 해제이며, 그 설정은 Omada 컨트롤러에서 적용됨.
 *    거리가 멀어지면 rate adaptation 이 자동으로 1~2Mbps 로 낮춰 링크를 유지한다.
 */
void WifiForceLowRateInit()
{
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  delay(100);
  esp_wifi_set_max_tx_power(80); // 20 dBm
}
