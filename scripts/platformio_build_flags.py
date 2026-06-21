import json
import os
import subprocess

Import("env")


def git_commit():
    value = os.environ.get("GITHUB_SHA")
    if value:
        return value[:12]
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short=12", "HEAD"],
            text=True,
        ).strip()
    except Exception:
        return "local"


def define_string(name, value):
    escaped = json.dumps(value)[1:-1]
    env.Append(CPPDEFINES=[(name, '\\"' + escaped + '\\"')])


define_string("BUILD_GIT_COMMIT", git_commit())
define_string(
    "BUILD_LIBRARY_SUMMARY",
    "pioarduino/platform-espressif32#55.03.38,"
    "arduino-esp32@3.3.8,"
    "HAS2_Wifi@v1.0.0,"
    "Adafruit_NeoPixel@1.15.5,"
    "ArduinoJson@7.4.3,"
    "IRremoteESP8266@2.9.0,"
    "Pangodream_18650_CL@e1be2aa,"
    "Nextion@vendor,"
    "SimpleTimer@vendor"
)
