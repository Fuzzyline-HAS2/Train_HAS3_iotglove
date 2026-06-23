@echo off
rem Auto-detect a DFRobot Beetle ESP32-C3 and flash the wifi_location build.
rem Double-click to run, or pass extra args (forwarded to the .ps1), e.g.:
rem   auto_flash_beetle.bat -Firmware "C:\path\to\firmware.factory.bin"
chcp 65001 >nul
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0auto_flash_beetle.ps1" %*
echo.
echo Watcher stopped.
pause
