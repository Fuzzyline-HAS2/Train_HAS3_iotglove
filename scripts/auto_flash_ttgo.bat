@echo off
rem Auto-detect a TTGO T8 v1.7.1 (CH9102 USB-UART) and flash the updated_IoTglove build.
rem Double-click to run, or pass extra args (forwarded to the .ps1), e.g.:
rem   auto_flash_ttgo.bat -Firmware "C:\path\to\firmware.factory.bin"
rem   auto_flash_ttgo.bat -VidPid "VID_10C4.*PID_EA60"   (CP2104 boards)
chcp 65001 >nul
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0auto_flash_ttgo.ps1" %*
echo.
echo Watcher stopped.
pause
