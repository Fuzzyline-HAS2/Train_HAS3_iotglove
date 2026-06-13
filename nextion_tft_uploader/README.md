# Nextion TFT 업로더

이 폴더는 ESP32를 통해 Nextion 디스플레이에 `.tft` 파일을 업로드하는 실행 패키지입니다.

폴더 전체를 다른 위치나 다른 PC로 옮겨도 사용할 수 있도록 모든 경로는 이 폴더 기준으로 동작합니다.

## 바로 사용하기

1. 업로드할 `.tft` 파일을 `tft` 폴더에 넣습니다.
2. Windows에서 `UPLOAD_NEXTION_TFT.bat`를 실행합니다.
3. 화면에 표시되는 TFT 파일 목록에서 업로드할 파일을 선택합니다.
4. 업로더가 ESP32 포트를 자동으로 찾습니다.
5. 감지된 포트가 맞는지 확인하면 업로드가 시작됩니다.
6. 완료되면 화면에 `업로드가 완료되었습니다.`라고 표시됩니다.

## 폴더 구성

```text
nextion_tft_uploader/
  UPLOAD_NEXTION_TFT.bat
  UPLOAD_NEXTION_TFT.sh
  README.md
  tft/
    glove.tft
    README.md
  tools/
    upload_nextion_tft.py
  docs/
    AI_AGENT_GUIDE.md
  logs/
```

## 다른 PC로 옮길 때

`nextion_tft_uploader` 폴더 전체를 복사하면 됩니다.

필요 조건:

- Windows PC
- Python 3 설치
- ESP32에 Nextion TFT 업로드 기능이 통합된 펌웨어가 업로드되어 있을 것
- ESP32가 USB로 PC에 연결되어 있을 것
- Nextion 디스플레이가 ESP32 Serial2에 연결되어 있을 것

Python이 없는 PC에서는 BAT가 친절한 안내 문구를 보여주고 멈춥니다.

## 기본 통신 설정

- PC와 ESP32: `921600`
- Nextion 기본 baud: `9600`
- Nextion 업로드 baud: `921600`
- 전송 chunk: `4096` bytes

## 고급 실행 예시

특정 포트로 업로드:

```powershell
.\UPLOAD_NEXTION_TFT.bat --port COM28
```

업로드 없이 ESP32 포트만 찾기:

```powershell
.\UPLOAD_NEXTION_TFT.bat --detect-only
```

첫 번째 TFT 파일을 바로 선택하고 질문 없이 진행:

```powershell
.\UPLOAD_NEXTION_TFT.bat --index 1 -y
```

색상 없이 실행:

```powershell
.\UPLOAD_NEXTION_TFT.bat --no-color
```
