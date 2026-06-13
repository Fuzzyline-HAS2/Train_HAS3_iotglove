#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

printf "\033[96m"
echo "============================================================"
echo " Nextion TFT 업로더"
echo "============================================================"
printf "\033[0m"
echo
echo "이 폴더는 다른 위치나 다른 PC로 옮겨도 그대로 사용할 수 있습니다."
echo "업로드할 TFT 파일은 아래 폴더에 넣어주세요."
echo
echo "TFT 폴더: $SCRIPT_DIR/tft"
echo

if command -v py >/dev/null 2>&1; then
  py -3 "$SCRIPT_DIR/tools/upload_nextion_tft.py" --folder "$SCRIPT_DIR/tft" "$@"
elif command -v python >/dev/null 2>&1; then
  python "$SCRIPT_DIR/tools/upload_nextion_tft.py" --folder "$SCRIPT_DIR/tft" "$@"
elif command -v python3 >/dev/null 2>&1; then
  python3 "$SCRIPT_DIR/tools/upload_nextion_tft.py" --folder "$SCRIPT_DIR/tft" "$@"
else
  printf "\033[93m"
  echo "Python 3을 찾지 못했습니다. Python 3 설치 후 다시 실행해주세요."
  printf "\033[0m"
  exit 1
fi

EXIT_CODE=$?
echo
if [ "$EXIT_CODE" -eq 0 ]; then
  printf "\033[92m"
  echo "============================================================"
  echo " 업로드 작업이 완료되었습니다."
  echo "============================================================"
  printf "\033[0m"
else
  printf "\033[91m"
  echo "============================================================"
  echo " 업로드 작업이 완료되지 않았습니다. 오류 내용을 확인해주세요."
  echo " 종료 코드: $EXIT_CODE"
  echo "============================================================"
  printf "\033[0m"
fi
exit "$EXIT_CODE"
