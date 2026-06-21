# Beetle C3 Location Firmware

This sketch is the Beetle ESP32-C3 companion firmware for HAS2 IoT Glove.

## Arduino IDE Build Settings

Use these board settings when compiling `wifi_location.ino` in Arduino IDE:

- Board: `DFRobot Beetle ESP32-C3`
- USB CDC On Boot: `Enabled`
- CPU Frequency: `160MHz (WiFi)`
- Flash Mode: `QIO`
- Flash Frequency: `80MHz`
- Flash Size: `4MB (32Mb)`
- Partition Scheme: `Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)`
- Core Debug Level: `None`

The partition scheme is required. The default partition gives only about
1.25MB for the app and this firmware exceeds that size because it includes BLE,
WiFi, HTTPS, JSON, and OTA support.

Do not use `No OTA` or `Huge APP` for release firmware. Those options can make a
large sketch compile, but they remove the OTA slot that this Beetle firmware
needs for GitHub Release OTA updates.

## Arduino CLI Equivalent

```powershell
arduino-cli compile `
  --fqbn "esp32:esp32:dfrobot_beetle_esp32c3:CDCOnBoot=cdc,PartitionScheme=min_spiffs,CPUFreq=160,FlashMode=qio,FlashFreq=80,FlashSize=4M,UploadSpeed=921600,DebugLevel=none" `
  ".\wifi_location"
```
