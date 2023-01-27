#include "iotglove.h"

/**
 * @brief 디스플레이 화면 초기화
 */
void DisplaySet()
{
    String cmd = "";
    if((String)(const char*)my["role"] != "tagger"){
        // life_chip 초기화
        cmd = "player.life_chip.val=" + (String)(const char*)my["life_chip"];
        sendCommand(cmd.c_str());
        // BatteryPack 초기화
        cmd = "player.BatteryPack.pic=player.battery_pic.val+" + (String)(const char*)my["battery_pack"];
        sendCommand(cmd.c_str());

        // LifeChip 초기화
        if((int)my["life_chip"] > 1){
            cmd = "player.LifeChip.pic=player.life_chip_pic.val+1";
            sendCommand(cmd.c_str());
        }
        else{
            cmd = "player.LifeChip.pic=player.life_chip_pic.val";
            sendCommand(cmd.c_str());
        }

        // level 초기화
        cmd = "player.level.val=" + (String)(const char*)my["lv"];
        sendCommand(cmd.c_str());

        // exp 초기화
        cmd = "player.exp.val=" + (String)(const char*)my["exp"];
        sendCommand(cmd.c_str());
    }
    else{
        // taken_chip 초기화
        cmd = "tagger.taken_chip.val=" + (String)(const char*)my["taken_chip"];
        sendCommand(cmd.c_str());

        // TakenChip 초기화
        if((int)my["taken_chip"] > 0){
            cmd = "tagger.TakenChip.pic=tagger.taken_chip_pic.val+1";
            sendCommand(cmd.c_str());
        }
        else{
            cmd = "tagger.TakenChip.pic=tagger.taken_chip_pic.val";
            sendCommand(cmd.c_str());
        }

        // level 초기화
        cmd = "tagger.level.val=" + (String)(const char*)my["lv"];
        sendCommand(cmd.c_str());

        // exp 초기화
        cmd = "tagger.exp.val=" + (String)(const char*)my["exp"];
        sendCommand(cmd.c_str());
    }
}

/**
 * @brief 디스플레이에 변화를 주거나 변화가 있을시 실행
 */
void DisplayCheck()
{ 
    if(MySerial2.available() > 0){
        String nextion_string = MySerial2.readStringUntil(' ');
        if(nextion_string.startsWith("H_")){
            nextion_string = nextion_string.substring(2);
            Serial.print("Nextion String : "); Serial.println(nextion_string);
            NextionReceived(&nextion_string);
        }
        while (MySerial2.available() > 0){
            MySerial2.read();
        }
    }
}

/**
 * @brief 디스플레이에서 오는 Serial을 확인
 * 
 * @param nextion_string Serial 문자열 데이터
 */
void NextionReceived(String* nextion_string)
{
    if(*nextion_string == "lifesend"){
        //TODO 타이머 사용하여 IR 송신하게 
        IrSend();
        PageChange("lifechip_send");
        delay(5000);
        PageChange("player");
    }
    else if(*nextion_string == "lifechip_receive"){
        //TODO 디스플레이에서 5초 타이머 or x 터치를 통해 lifechip_receive 전송
        has2wifi.Send((const char *)tag["device_name"], "life_chip", "-1");
        has2wifi.Send((const char *)my["device_name"], "life_chip", "+1");

        if((String)(const char*)my["role"] == "ghost"){
            has2wifi.Send((const char *)tag["device_name"], "exp", "+100"); 
            has2wifi.Send((String)(const char*)my["device_name"], "role", "revival");
        }
        else if((String)(const char*)my["role"] == "player"){
            PageChange("player");
            ir_receive_timer.enable(ir_receive_timer_id);
        }
    }
    else if(*nextion_string == "hacking"){
        //TODO hacking 시리얼을 hacking 시작과 동시에 보냄
        hacking = true;
        ir_receive_timer.disable(ir_receive_timer_id);
    }
    else if(*nextion_string == "die"){
        hacking = false;
        has2wifi.Send((String)(const char*)my["tagger_name"], "taken_chip", "+1");
        has2wifi.Send((String)(const char*)my["tagger_name"], "exp", "+100");
        has2wifi.Send((String)(const char*)my["device_name"], "life_chip", "-1");

        if((int)my["life_chip"] > 1){
            has2wifi.Send((String)(const char*)my["device_name"], "role", "revival");
        }

    }
    else if(*nextion_string == "revival"){   // revival 종료
        has2wifi.Send((String)(const char*)my["device_name"], "exp", "+45");
        has2wifi.Send((String)(const char*)my["device_name"], "role", "player");
        pixels.lightColor(green);
        revival = false;
        ir_receive_timer.enable(ir_receive_timer_id);
        PageChange("player");
    }
    else if(*nextion_string == "messege_exit"){
        if((String)(const char*)my["role"] == "player"){
            PageChange("player");
        }
        else if((String)(const char*)my["role"] == "ghost"){
            Serial.println("messege ghost");
            PageChange("ghost");
        }

        has2wifi.Send((String)(const char*)my["device_name"], "message_sender", "no");
    }

    //BUG 한사람당 한번만 보낼 수 있는 버그 수정 필요
    else if((*nextion_string).startsWith("msg_")){
        String message_code = (*nextion_string).substring(4);
        String message_sender = (String)(const char *)my["device_name"];
        String group = message_sender.substring(0, 2);
        for (int i = 1; i < 9; ++i){
            String receive_player_name = group + "P" + String(i);
            if(receive_player_name == (String)(const char*)my["device_name"]){
                has2wifi.Send((String)(const char*)my["device_name"], "message_sender", "no");
            }
            else if(receive_player_name != (String)(const char*)my["device_name"]){
                has2wifi.Send(receive_player_name, "message_sender", (String)(const char*)my["player_name"]);
                has2wifi.Send(receive_player_name, "message_code", message_code);
            }
        }
    }
}

void PageChange(String page)
{
    static String cur_page = "";

    if(cur_page != page){
        String cmd = "page " + page;
        sendCommand(cmd.c_str());
        cur_page = page;
    }
}