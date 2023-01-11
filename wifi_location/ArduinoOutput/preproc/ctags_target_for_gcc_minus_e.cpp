# 1 "c:\\Users\\teamh\\OneDrive\\바탕 화면\\device_code\\iotglove\\wifi_location\\wifi_location.ino"
/**

 * @brief IoT글러브에 들어가는 Beetle 위치측정 코드

 *

 * 동작 : 와이파이 스캔을 통해 자신의 위치에서 가장 가까운 공유기의 이름을

 *        1. Serial을 통해 Bettle에서 TTGO로 전송

 *        2. 현재 자신의 위치와 Serial 통신을 통해 받은 위치가 다르다면

 *        3. 와이파이를 통해 TTGO에서 메인프로그램으로 전송

 *

 *

 */
# 12 "c:\\Users\\teamh\\OneDrive\\바탕 화면\\device_code\\iotglove\\wifi_location\\wifi_location.ino"
# 13 "c:\\Users\\teamh\\OneDrive\\바탕 화면\\device_code\\iotglove\\wifi_location\\wifi_location.ino" 2




HardwareSerial MySerial1(1); // TTGO

String cur_wifi_name;
String first_wifi_name;
bool send_ttgo;
int loop_num;

void setup()
{
    Serial.begin(115200);

    MySerial1.begin(115200, 0x800001c, 6, 5); // Beetle과 UART 통신 연결 세팅

    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);

    pinMode(10,0x03);
    digitalWrite(10,0x0);
    Serial.println("Setup done");
}

void loop()
{
    digitalWrite(10,0x1);
    int n = WiFi.scanNetworks();
    static int wifi_stack = 0;
    int cur_rssi = -100;

    // Serial.print("network : "); Serial.println(n);
    FirstRssi(n);
}

void FirstRssi(int scan_network)
{
    int network_num = 0;
    int32_t first_rssi = -150;

    for (int i = 0; i < scan_network; ++i)
    {
        if(WiFi.SSID(i).startsWith("HAS2") && WiFi.RSSI(i) > -50){
            Serial.print(WiFi.SSID(i));
            Serial.print(" [");
            Serial.print(WiFi.RSSI(i));
            Serial.println("]");
            Serial.println();


            if (WiFi.RSSI(i) > first_rssi)
            {
                first_rssi = WiFi.RSSI(i);
                first_wifi_name = WiFi.SSID(i);
            }
        }
    }

    if(cur_wifi_name != first_wifi_name){
        send_ttgo = false;
        loop_num = 0;
        cur_wifi_name = first_wifi_name;
    }
    else{
        if (!send_ttgo && ++loop_num > 1)
        {
            send_ttgo = true;
            // Serial.println(cur_wifi_name);
            if(cur_wifi_name.startsWith("HAS2")){
                Serial.print("Best : "); Serial.println(cur_wifi_name);
                Serial.println();
                MySerial1.print(cur_wifi_name);
            }
        }
    }
}
