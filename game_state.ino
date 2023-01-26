#include "iotglove.h"

/**
 * @brief DB gamestate가 setting 일 때 동작하는 코드
 */
void SettingFunc()
{
    game_state = setting;
    MySerial1.print("setting ");

    motor_on = false;
    MotorStop();
    if(neopixel_timer.isEnabled(neopixel_timer_id)){
        neopixel_timer.deleteTimer(neopixel_timer_id);
    }
    ir_receive_timer.disable(ir_receive_timer_id);
    pixels.lightColor(white);
    ledcWrite(5, 0);

    BatteryCheck();
    
    PageChange("0");
    sendCommand("sleep=1");
}

/**
 * @brief DB gamestate가 ready 일 때 동작하는 코드
 */

void ReadyFunc()
{
    game_state = ready;
    MySerial1.print("ready ");

    motor_on = false;
    MotorStop();
    if(neopixel_timer.isEnabled(neopixel_timer_id)){
        neopixel_timer.deleteTimer(neopixel_timer_id);
    }
    ir_receive_timer.disable(ir_receive_timer_id);
    sendCommand("sleep=0");
    PageChange("before_tagger");
    pixels.lightColor(red);
}

/**
 * @brief DB gamestate가 activate 일 때 반복 동작하는 코드
 */
void ActivateFunc()
{
    // 디스플레이 변경 체크
    DisplayCheck();

    // 진동모터
    if(motor_on && ((String)(const char*)my["role"] != "tagger" || (String)(const char*)my["role"] != "neutral")){
        MotorOn(vibration_pattern_2, ARRAYINDEX(vibration_pattern_2));
    }
    else{
        MotorStop();
    }

    // 술래는 지속적으로 IR 송신
    if((String)(const char *)my["role"] == "tagger" && (int)my["taken_chip"] < (int)my["max_taken_chip"] && (String)(const char *)my["device_state"] == "activate") { IrSend(); }
}

/**
 * @brief DB gamestate가 activate 일 때 한번 동작하는 코드
 */
void ActivateRunOnce()
{
    game_state = activate;

    MySerial1.print("activate ");
    ledcWrite(5, 0);

    if((String)(const char*)my["role"] == "player" || (String)(const char*)my["role"] == "ghost"){
        ir_receive_timer.enable(ir_receive_timer_id);
    }
    DisplaySet();
    String cmd = "revival.revival_time.val=" + (String)(const char*)my["revival_time"];
    sendCommand(cmd.c_str());
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
            if((String)(const char*)my["role"] == "player"){
                PageChange("player");
                pixels.lightColor(green);
                ir_receive_timer.enable(ir_receive_timer_id);
            } 
            else if((String)(const char*)my["role"] == "tagger"){
                if(neopixel_timer.isEnabled(neopixel_timer_id)){
                    neopixel_timer.deleteTimer(neopixel_timer_id) ;
                }
                ir_receive_timer.disable(ir_receive_timer_id);
                PageChange("tagger");
                pixels.lightColor(purple);
            }
            else if((String)(const char *)my["role"] == "revival"){
                ir_receive_timer.disable(ir_receive_timer_id);
                PageChange("revival");
                pixels.lightColor(yellow);
                revival = true;
            }
            else if((String)(const char *)my["role"] == "ghost"){
                PageChange("ghost");
                pixels.lightColor(blue);
                ir_receive_timer.enable(ir_receive_timer_id);
            }
        }
        else if((String)(const char *)my["device_state"] == "player_win"){
            MotorStop();
            PageChange("win_lose");
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
            MotorStop();
            PageChange("win_lose");
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
                neopixel_timer_id = neopixel_timer.setInterval(400, tagger_blink);
            }
            sendCommand("sleep=0");
            PageChange("tagger");
        }
        
        else if((String)(const char*)my["device_state"] == "photo"){
            game_state = setting;
            MySerial1.print("setting ");
            MotorStop();
            sendCommand("sleep=0");
            if((String)(const char*)my["role"] == "player" || (String)(const char*)my["role"] == "ghost"){
                PageChange("player");
                pixels.lightColor(green);
            }
            else if((String)(const char*)my["role"] == "tagger"){
                PageChange("tagger");
                pixels.lightColor(purple);
            }
        }
    }

    // role에 변화가 있을 시
    if((String)(const char *)my["role"] != (String)(const char *)cur["role"]){
        if(game_state == activate){
            if((String)(const char *)my["role"] == "revival"){
                ir_receive_timer.disable(ir_receive_timer_id);
                PageChange("revival");
                pixels.lightColor(yellow);
                revival = true;
            }
            else if((String)(const char *)my["role"] == "ghost"){
                PageChange("ghost");
                pixels.lightColor(blue);
                ir_receive_timer.enable(ir_receive_timer_id);
            }
        }
    }

    // life_chip에 변화가 있을 시 
    if((int)my["life_chip"] != (int)cur["life_chip"]){
        if((String)(const char*)my["role"] == "player" || (String)(const char*)my["role"] == "ghost" || (String)(const char*)my["role"] == "revival"){
            if((int)my["life_chip"] == 1){
                cmd = "player.LifeChip.pic=player.life_chip_pic.val";
                sendCommand(cmd.c_str());
                if((String)(const char*)my["role"] == "ghost"){
                    has2wifi.Send((String)(const char*)my["device_name"], "role", "revival");
                }
                if((int)cur["life_chip"] > (int)my["life_chip"]){
                    has2wifi.Send((String)(const char*)my["device_name"], "role", "revival");
                }
            }
            else if((int)my["life_chip"] > 1){
                cmd = "player.LifeChip.pic=player.life_chip_pic.val+1";
                sendCommand(cmd.c_str());
                if((int)cur["life_chip"] > (int)my["life_chip"]){
                    has2wifi.Send((String)(const char*)my["device_name"], "role", "revival");
                }
            }
            else if((int)my["life_chip"] < 1){
                has2wifi.Send((String)(const char*)my["device_name"], "role", "ghost");
                Serial.println("send ghost ");
            }

            cmd = "player.life_chip.val=" + (String)(const char*)my["life_chip"];
            sendCommand(cmd.c_str());
            
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

    // vibe에 변화가 있을 시
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
        if((String)(const char*)my["message_sender"] == "no") {cur = my; return ;}
        if(((String)(const char*)my["role"] == "player" || (String)(const char*)my["role"] == "ghost") && ((String)(const char*)my["game_state"] == "activate")){
            if(((String)(const char*)my["message_sender"] != (String)(const char*)my["player_name"]) && !revival && !hacking){
                if(revival || hacking){
                    has2wifi.Send((String)(const char*)my["device_name"], "message_sender", "no");
                    cur = my;
                    return ;
                }
                PageChange("msg_receive");
                cmd = "sender.pic=sender_pic.val+" + (String)(const char*)my["message_sender"];
                sendCommand(cmd.c_str());
                cmd = "code.pic=code_pic.val+" + (String)(const char*)my["message_code"];
                sendCommand(cmd.c_str());
            }
        }
        else{
            has2wifi.Send((String)(const char*)my["device_name"], "message_sender", "no");
        }
    }

    if((int)my["skill_point"] != (int)cur["skill_point"]){
        
    }
    Serial.println("Data Change");
    
    cur = my;

    if(display_change++ > 2){
        display_change = 0;
        DisplaySet();
    }
}