# AI Agent Guide: Nextion TFT Uploader

이 문서는 다른 AI agent가 이 패키지를 이해하고 사용할 수 있도록 작성된 설명서입니다.

## 패키지 목적

`nextion_tft_uploader`는 PC에서 ESP32를 중계기로 사용해 Nextion 디스플레이에 `.tft` 파일을 업로드하는 독립 실행 폴더입니다.

이 패키지는 위치 독립적으로 동작해야 합니다. 사용자가 폴더를 다른 위치나 다른 PC로 옮겨도 `UPLOAD_NEXTION_TFT.bat`는 자기 폴더 기준으로 `tools/upload_nextion_tft.py`와 `tft/`를 찾습니다.

## 사용자가 실행해야 하는 파일

Windows:

```powershell
.\UPLOAD_NEXTION_TFT.bat
```

Bash:

```bash
./UPLOAD_NEXTION_TFT.sh
```

## 중요한 경로 규칙

- 절대 경로를 추가하지 마세요.
- 업로드할 TFT 파일 기본 위치는 항상 `nextion_tft_uploader/tft/`입니다.
- Python 스크립트는 `Path(__file__).resolve()`를 기준으로 기본 `tft` 폴더를 찾습니다.
- BAT는 `%~dp0tools\upload_nextion_tft.py`와 `%~dp0tft`를 사용합니다.
- SH는 자기 파일 위치인 `SCRIPT_DIR`를 사용합니다.

## 동작 순서

1. `tft/` 폴더에서 `.tft` / `.TFT` 파일을 찾습니다.
2. 파일이 하나면 자동 선택합니다.
3. 파일이 여러 개면 번호 선택 화면을 보여줍니다.
4. Windows COM 포트를 검색합니다.
5. 각 포트를 `921600` baud로 열고 `NXUP_HELLO`를 보냅니다.
6. `NXUP_READY` 응답이 있는 포트를 ESP32 업로드 릴레이로 판단합니다.
7. 사용자에게 TFT 파일과 ESP32 포트를 보여주고 업로드 여부를 묻습니다.
8. 확인되면 `BEGIN`, `DATA`, `DATA_OK`, `DONE` 프로토콜로 업로드합니다.
9. 업로드가 끝나면 사용자에게 완료 메시지를 보여줍니다.

## ESP32 펌웨어 요구사항

ESP32에는 `updated_IoTglove`에 통합된 `nextion_tft_upload.ino`가 업로드되어 있어야 합니다.

필수 동작:

- PC USB Serial baud: `921600`
- PC가 `NXUP_HELLO`를 보내면 ESP32가 `NXUP_READY 1`로 응답
- 업로드 시작 시 Nextion에 `whmi-wris` 명령 전송
- 파일 chunk 수신 후 Nextion ACK를 확인하고 PC에 `DATA_OK` 응답

## Python 옵션

- `--port auto`: 기본값. ESP32 포트 자동 검색
- `--port COM28`: 지정 포트 사용
- `--folder PATH`: TFT 파일을 찾을 폴더
- `--tft PATH`: 특정 TFT 파일 직접 지정
- `--index N`: N번째 TFT 파일 자동 선택
- `--detect-only`: 업로드 없이 포트 감지만 수행
- `-y` / `--yes`: 사용자 확인 없이 진행
- `--no-color`: 색상 출력 끄기
- `--verbose-scan`: 포트 검색 실패 이유 표시

## 검증 명령

파일 선택만 확인:

```powershell
python tools\upload_nextion_tft.py --folder tft --index 1 --list-only
```

포트 감지만 확인:

```powershell
python tools\upload_nextion_tft.py --folder tft --index 1 --detect-only -y
```

실제 업로드:

```powershell
python tools\upload_nextion_tft.py --folder tft --port auto
```

## 정상 완료 기준

정상 완료 로그에는 아래 내용이 포함됩니다.

```text
진행률 100%
업로드가 완료되었습니다.
```

`DONE no_ready_notice`가 표시될 수 있습니다. 이것은 Nextion의 마지막 ready 알림을 받지 못했다는 의미이며, 파일 전송이 100% 완료되고 완료 문구가 표시되면 PC 측 업로드는 성공으로 판단합니다.

## 작업 시 주의사항

- 이 패키지에 절대 경로를 하드코딩하지 마세요.
- `tft/` 폴더 안의 사용자 파일을 삭제하지 마세요.
- Serial Monitor나 다른 업로더가 COM 포트를 잡고 있으면 포트 감지나 업로드가 실패합니다.
- 다른 PC에서 Python이 없으면 BAT는 설치 안내를 표시하고 종료합니다.
