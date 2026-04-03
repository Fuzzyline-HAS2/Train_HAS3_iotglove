# IoT Glove

ESP32 기반 IoT 장갑 펌웨어 — HAS2 배드랜드 서바이벌 게임용

---

## 개요

플레이어가 착용하는 IoT 장갑으로, Wi-Fi를 통해 서버와 실시간 통신하며 게임 상태를 관리합니다.  
IR 송수신으로 술래의 해킹 및 플레이어 간 생명칩 교환을 처리하고, 네오픽셀·진동모터·부저·디스플레이로 게임 상황을 피드백합니다.

---

## 하드웨어

| 부품 | 핀 | 용도 |
|---|---|---|
| ESP32 (TTGO T1) | — | 메인 MCU |
| NeoPixel | 25 | 역할별 LED 색상 표시 |
| IR 수신부 | 27 | IR 신호 수신 |
| IR 송신부 | 4 | IR 신호 송신 |
| Nextion 디스플레이 | TX:33 / RX:39 | 게임 UI |
| Beetle (보조 MCU) | TX:32 / RX:36 | Wi-Fi 스캔 (위치 측정) |
| 진동모터 | 12/13/14 | 햅틱 피드백 |
| 피에조 부저 | 15 | 음향 피드백 |
| QRD1114 | 34 | 근접 감지 |
| 18650 배터리 | 35 | 전원 / 잔량 측정 |

---

## 게임 역할

| 역할 | LED 색상 | 설명 |
|---|---|---|
| `player` | 초록 | 생존자. IR 수신 대기, 해킹 당할 수 있음 |
| `tagger` | 보라 | 술래. IR 송신으로 플레이어 해킹 |
| `ghost` | 파랑 | 탈락 상태. 생명칩 교환 가능 |
| `revival` | 노랑 | 부활 대기 상태 |

---

## 주요 기능

### IR 해킹 시스템
- 술래는 `IrSend()`로 NEC 프로토콜 IR 신호를 지속 송신
- 플레이어는 `HACK_THRESHOLD`(기본 3)회 연속 수신 시 해킹 성공
- **술래의 IR 송신 duty cycle을 `IR_TAGGER_DUTY`(기본 5%)로 낮춰 도달 거리를 약 5cm로 제한** → 실제로 바짝 붙어야 해킹 가능

### 생명칩 교환
- 유령(ghost)이 플레이어에게 IR 신호 송신 → 생명칩 수신
- 술래와 달리 50% duty cycle로 일반 거리에서 동작

### Wi-Fi 위치 추적
- Beetle이 주변 `badland_*` SSID를 스캔해 UART로 전달
- `has2wifi.Send()`로 서버 DB에 현재 위치 업로드

### OTA 업데이트
- SecureOTA 라이브러리 사용
- DB의 `device_state`가 `"github"`로 변경되면 자동 업데이트 체크
- 서명 검증 후 적용 (`update.sig` / `update.bin`)

### 게임 상태 흐름
```
setting → ready → activate → (player_win / player_lose)
```

---

## 상수 튜닝

`updated_IoTglove.h`에서 조정

```cpp
#define HACK_THRESHOLD   3   // 해킹 성공까지 필요한 연속 IR 수신 횟수
#define IR_TAGGER_DUTY   5   // 술래 IR 송신 duty cycle (%) — 낮을수록 거리 짧아짐 (기본값 50)
```

---

## 파일 구조

```
updated_IoTglove.ino   — setup() / loop(), 초기화
game_state.ino         — 게임 상태별 로직 (setting / ready / activate)
sensor.ino             — IR / QRD1114 / 배터리 / 모터 / 네오픽셀
display.ino            — Nextion 디스플레이 제어
timer.ino              — SimpleTimer 기반 타이머 설정
updated_IoTglove.h     — 전역 변수 / 상수 선언
library_and_pin.h      — 라이브러리 import / 핀 정의
secrets.h              — Wi-Fi 자격증명 (git 미포함)
scripts/               — OTA 서명 및 배포 스크립트 (Python)
```

---

## 빌드 환경

- Arduino IDE / arduino-cli
- Board: `esp32:esp32:ttgo-t1`
- 주요 라이브러리: `IRremoteESP8266`, `Adafruit NeoPixel`, `Nextion`, `SimpleTimer`, `Pangodream_18650_CL`, `SecureOTA`, `HAS2_Wifi`

---

## 버전

| 버전 | 내용 |
|---|---|
| v6 (현재) | IR duty cycle 제어로 술래 해킹 거리 5cm 제한 |
| v5 | 게임 로직 버그 수정 및 기능 추가 |
| v4 | SecureOTA 연결, ESP32 core v3 ledc API 마이그레이션 |

---

© 2022–2026 YuBin Kim / HAS2
