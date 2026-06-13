@echo off
setlocal
chcp 65001 >nul
title Nextion TFT Uploader
color 0B

cd /d "%~dp0"

echo.
echo ============================================================
echo  Nextion TFT Uploader
echo ============================================================
echo.
echo  This folder is portable.
echo  Put TFT files into the tft folder next to this BAT file.
echo.

set "PYTHON_CMD="
where py >nul 2>nul
if not errorlevel 1 set "PYTHON_CMD=py -3"

if not defined PYTHON_CMD (
  where python >nul 2>nul
  if not errorlevel 1 set "PYTHON_CMD=python"
)

if not defined PYTHON_CMD (
  color 0E
  echo Python 3 was not found.
  echo.
  echo Please install Python 3, then run this BAT file again.
  echo https://www.python.org/downloads/
  echo.
  pause
  exit /b 1
)

%PYTHON_CMD% "%~dp0tools\upload_nextion_tft.py" --folder "%~dp0tft" %*
set "EXIT_CODE=%ERRORLEVEL%"

echo.
if "%EXIT_CODE%"=="0" (
  color 0A
  echo ============================================================
  echo  Task finished.
  echo ============================================================
) else (
  color 0C
  echo ============================================================
  echo  Task did not finish. Please check the error message above.
  echo  Exit code: %EXIT_CODE%
  echo ============================================================
)
echo.
pause
exit /b %EXIT_CODE%
