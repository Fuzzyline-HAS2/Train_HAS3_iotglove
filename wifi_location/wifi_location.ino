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

#include "WiFi.h"

#define SERIAL1_RX_PIN  6
#define SERIAL1_TX_PIN  5

HardwareSerial MySerial1(1);    // TTGO

typedef enum GameState {setting, ready, activate};
GameState game_state = setting;

String cur_wifi_name;
String first_wifi_name;
bool send_ttgo;
int loop_num;

void setup()
{
    Serial.begin(115200);

    MySerial1.begin(115200, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN); // Beetle과 UART 통신 연결 세팅

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Beetle 내부 LED 핀번호 : 10
    pinMode(10,OUTPUT);
    digitalWrite(10,LOW);
    Serial.println("Setup done");
    
     MySerial1.print("reset ");
}

void loop()
{
    digitalWrite(10,HIGH);
    int n = WiFi.scanNetworks();
    static int wifi_stack = 0;
    int cur_rssi = -100;

    if(MySerial1.available() > 0){
        String serial1_data = MySerial1.readStringUntil(' ');
        Serial.println(serial1_data);

        if(serial1_data == "activate"){
            game_state = activate;
        }
        else if(serial1_data == "setting"){
            game_state = setting;
        }
        else if(serial1_data == "ready"){
            game_state = ready;
        }

        while(MySerial1.available() > 0){
            MySerial1.read();
        }
    }
    
    if(game_state == activate){
        FirstRssi(n);
    }
    else{
        send_ttgo = false;
        loop_num = 0;
        cur_wifi_name = first_wifi_name;
    }
    
}

void FirstRssi(int scan_network)
{
    int network_num = 0;
    int32_t first_rssi = -150;

    for (int i = 0; i < scan_network; ++i)
    {
        if(WiFi.SSID(i).startsWith("HAS2") && WiFi.RSSI(i) > -50){
            Serial.print(WiFi.SSID(i));Serial.print(" ["); Serial.print(WiFi.RSSI(i));Serial.println("]");Serial.println();
     
            if (WiFi.RSSI(i) > first_rssi){      
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
                MySerial1.print(cur_wifi_name + " ");
            }
        }
    }
}
