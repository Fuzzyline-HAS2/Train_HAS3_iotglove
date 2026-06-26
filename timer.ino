#include "Train_HAS3_iotglove.h"
/**
 * @brief millis 기반 타이머 세팅
 */
void TimerInit()
{
  // IRrecv는 프레임 캡처 후 resume() 전까지 다음 수신을 멈추므로, 자주 폴링해야
  // STOP 구간에 프레임이 갇혀 누락되는 일이 없다. (지연 단축이 아니라 수신 신뢰성 목적)
  ir_receive_timer_id = ir_receive_timer.setInterval(20, IrReceive);
  ir_receive_timer.disable(ir_receive_timer_id);
  wifi_timer_id = wifi_timer.setInterval(1000, WifiTimerFunc);
  battery_timer_id = battery_timer.setInterval(60000, BatteryCheck);
}

/**
 * @brief Loop문에서 지속적으로 타이머의 시간을 체크
 */
void TimerRun()
{
  ir_receive_timer.run();
  wifi_timer.run();
  neopixel_timer.run();
  battery_timer.run();
}

void WifiTimerFunc()
{
  // has2wifi.Connect("city");
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(glove_ssid, glove_pass); // badland_auto 다이렉트 재연결 (rate는 연결 이벤트에서 재적용)
  }
  has2wifi.Loop(DataChange);

}
