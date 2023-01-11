#include "iotglove.h"

/**
 * @brief DB gamestate가 setting 일 때 동작하는 코드
 */
void SettingFunc()
{
    game_state = setting;

    MotorStop();
    neopixel_timer.deleteTimer(neopixel_timer_id);
    pixels.lightColor(white);
    ledcWrite(5, 0);

    BatteryCheck();
    
    sendCommand("page 0");
    sendCommand("sleep=1");
}

/**
 * @brief DB gamestate가 ready 일 때 동작하는 코드
 */

void ReadyFunc()
{
    game_state = ready;

    MotorStop();
    neopixel_timer.deleteTimer(neopixel_timer_id);
    sendCommand("sleep=0");
    sendCommand("page before_tagger");
    pixels.lightColor(red);
}

/**
 * @brief DB gamestate가 activate 일 때 반복 동작하는 코드
 */
void ActivateFunc()
{
    // 디스플레이 변경 체크
    DisplayCheck();

    // 술래는 지속적으로 IR 송신
    if((String)(const char *)my["role"] == "tagger" && (int)my["taken_chip"] < (int)my["max_taken_chip"]) { IrSend(); }
    
    // 가장 가까운 와이파이 위치 정보를 Beetle을 통해 받음
    if (MySerial1.available()){
        if(game_state == activate){
            wifi_name = MySerial1.readStringUntil(' ');
            if(wifi_name.startsWith("HAS2")){
                Serial.println(wifi_name);
                has2wifi.Send((String)(const char*)my["device_name"], "location", wifi_name);
            }
        }
        else{
            MySerial1.read();
        }
    }
}

/**
 * @brief DB gamestate가 activate 일 때 한번 동작하는 코드
 */
void ActivateRunOnce()
{
    game_state = activate;
    ledcWrite(5, 0);
}

/**
 * @brief DB에 변화가 있을 시 동작 
 */
void DataChange()
{
    static StaticJsonDocument<1000> cur;

    String cmd = "";

    // game_state에 변화가 있을 시
    if((String)(const char *)my["game_state"] != (String)(const char *)cur["game_state"]){
        if ((String)(const char *)my["game_state"] == "setting"){
            SettingFunc();
        }
        else if ((String)(const char *)my["game_state"] == "ready"){
            ReadyFunc();
        }
        else if ((String)(const char *)my["game_state"] == "activate"){
            ActivateRunOnce();
        }
    }

    // device_state에 변화가 있을 시
    if((String)(const char *)my["device_state"] != (String)(const char *)cur["device_state"]){
        if((String)(const char *)my["device_state"] == "activate"){
            cmd = "start.player_name.val=" + (String)(const char*)my["player_name"];
            sendCommand(cmd.c_str());
            sendCommand("sleep=0");
            sendCommand("dims=100");
            DisplaySet();
        }
        else if((String)(const char *)my["device_state"] == "player_win"){
            sendCommand("page win_lose");
            if((String)(const char*)my["role"] == "player"){
                cmd = "WinLose.pic=win_lose_pic.val"; // 생존자 승리
                sendCommand(cmd.c_str());
            }
            else if((String)(const char*)my["role"] == "tagger"){
                cmd = "WinLose.pic=win_lose_pic.val+3"; // 술래 패배
                sendCommand(cmd.c_str());
            }
        }
        else if((String)(const char *)my["device_state"] == "player_lose"){
            sendCommand("page win_lose");
            if((String)(const char*)my["role"] == "player"){
                cmd = "WinLose.pic=win_lose_pic.val+1"; // 생존자 패배
                sendCommand(cmd.c_str());
            }
            else if((String)(const char*)my["role"] == "tagger"){
                cmd = "WinLose.pic=win_lose_pic.val+2"; // 술래 승리
                sendCommand(cmd.c_str());
            }
        }
        
        else if((String)(const char*)my["role"] == "tagger" && (String)(const char*)my["device_state"] == "blink"){
            if(!neopixel_timer.isEnabled(neopixel_timer_id)){
                neopixel_timer_id = neopixel_timer.setInterval(800, tagger_blink);
            }
            sendCommand("sleep=0");
            sendCommand("dims=100");
            DisplaySet();
            sendCommand("page tagger");
        }
        
        else if((String)(const char*)my["device_state"] == "photo"){
            game_state = setting;
            MotorStop();
            if((String)(const char*)my["role"] == "player" || (String)(const char*)my["role"] == "ghost"){
                sendCommand("page player");
                pixels.lightColor(green);
            }
            else if((String)(const char*)my["role"] == "tagger"){
                sendCommand("page tagger");
                pixels.lightColor(purple);
            }
        }
    }

    // role에 변화가 있을 시
    if((String)(const char *)my["role"] != (String)(const char *)cur["role"]){
    if(game_state == activate){
        if((String)(const char*)my["role"] == "player"){
                sendCommand("page player");
                pixels.lightColor(green);
            } 
            else if((String)(const char*)my["role"] == "tagger"){
                if(neopixel_timer.isEnabled(neopixel_timer_id)){
                    neopixel_timer.deleteTimer(neopixel_timer_id) ;
                }
                sendCommand("page tagger");
                pixels.lightColor(purple);
            }
            if((String)(const char *)my["role"] == "revival"){
                if(ir_receive_timer.isEnabled(ir_receive_timer_id)){
                    ir_receive_timer.deleteTimer(ir_receive_timer_id);
                }
                sendCommand("page revival");
                pixels.lightColor(yellow);
                revival = true;
            }
            else if((String)(const char *)my["role"] == "ghost"){
                sendCommand("page ghost");
                pixels.lightColor(blue);
            }
        }
    }

    // life_chip에 변화가 있을 시
    if((int)my["life_chip"] != (int)cur["life_chip"]){
        cmd = "player.life_chip.val=" + (String)(const char*)my["life_chip"];
        sendCommand(cmd.c_str());
        if((int)my["life_chip"] == 1){
            cmd = "player.LifeChip.pic=player.life_chip_pic.val";
            sendCommand(cmd.c_str());
            if((String)(const char*)my["role"] == "ghost"){
                has2wifi.Send((String)(const char*)my["device_name"], "role", "revival"); 
            }
            if((int)my["life_chip"] < (int)cur["life_chip"]){
                has2wifi.Send((String)(const char*)my["device_name"], "role", "revival");
            }
        }
        else if((int)my["life_chip"] > 1){
            cmd = "player.LifeChip.pic=player.life_chip_pic.val+1";
            sendCommand(cmd.c_str());
            if((int)my["life_chip"] < (int)cur["life_chip"]){
                has2wifi.Send((String)(const char*)my["device_name"], "role", "revival");
            }
        }
        else if((int)my["life_chip"] < 1){
            has2wifi.Send((String)(const char*)my["device_name"], "role", "ghost");
        }
    }

    // taken_chip에 변화가 있을 시
    if((int)my["taken_chip"] != (int)cur["taken_chip"]){
        cmd = "tagger.taken_chip.val=" + (String)(const char*)my["taken_chip"];
        sendCommand(cmd.c_str());
        if((int)my["taken_chip"] == 0){
            cmd = "tagger.TakenChip.pic=taken_chip_pic.val";
            sendCommand(cmd.c_str());
        }
        else if((int)my["taken_chip"] > 0){
            cmd = "tagger.TakenChip.pic=taken_chip_pic.val+1";
            sendCommand(cmd.c_str());
        }
    }

    // battery_pack에 변화가 있을 시
    if((int)my["battery_pack"] != (int)cur["battery_pack"]){
        cmd = "player.BatteryPack.pic=player.battery_pic.val+" + (String)(const char*)my["battery_pack"];
        sendCommand(cmd.c_str());
    }

    // exp에 변화가 있을 시
    if((int)my["exp"] != (int)cur["exp"]){
        cmd = (String)(const char*)my["role"] + ".exp.val=" + (String)(const char*)my["exp"];
        sendCommand(cmd.c_str());
    }

    // lv에 변화가 있을 시
    if((int)my["lv"] != (int)cur["lv"]){
        if((String)(const char*)my["role"] == "player" || (String)(const char*)my["role"] == "ghost"){
            cmd = "player.level.val=" + (String)(const char*)my["lv"];
            sendCommand(cmd.c_str());
        }
        if((String)(const char*)my["role"] == "tagger"){
            cmd = "tagger.level.val=" + (String)(const char*)my["lv"];
            sendCommand(cmd.c_str());
        }
    }

    if((int)my["vibe"] != (int)cur["vibe"]){
        if((String)(const char*)my["role"] != "tagger" && (String)(const char*)my["game_state"] == "activate"){
            if((int)my["vibe"] == 2){
                motor_on = true;
            }
            else if((int)my["vibe"] == 1){

            }
            else if((int)my["vibe"] == 0){
                motor_on = false;
                MotorStop();
            }
        }
    }

    if((String)(const char*)my["message_sender"] != (String)(const char*)cur["message_sender"]){
        if((String)(const char*)my["message_sender"] == "no") { Serial.println("return"); return ;}
        if((String)(const char*)my["role"] != "tagger" && ((String)(const char*)my["game_state"] == "activate")){
            if(((String)(const char*)my["message_sender"] != (String)(const char*)my["player_name"]) && !revival){
                sendCommand("page msg_receive");
                cmd = "sender.pic=sender_pic.val+" + (String)(const char*)my["message_sender"];
                sendCommand(cmd.c_str());
                cmd = "code.pic=code_pic.val+" + (String)(const char*)my["message_code"];
                sendCommand(cmd.c_str());
            }
        }
    }

    if((int)my["skill_point"] != (int)cur["skill_point"]){
        
    }
    Serial.println("Data Change");
    
    cur = my;
}
