#include "updated_IoTglove.h"

int GameJsonInt(const char *key, int fallback)
{
    if (my[key].isNull())
    {
        return fallback;
    }
    return my[key].as<int>();
}

const char *GameJsonText(const char *key)
{
    const char *value = my[key].as<const char *>();
    return value == NULL ? "" : value;
}

bool TextEquals(const char *value, const char *expected)
{
    if (value == NULL)
    {
        value = "";
    }
    if (expected == NULL)
    {
        expected = "";
    }
    return strcmp(value, expected) == 0;
}

const char *CurrentRole()
{
    return GameJsonText("role");
}

const char *CurrentDeviceState()
{
    return GameJsonText("device_state");
}

const char *CurrentGameState()
{
    return GameJsonText("game_state");
}

bool CurrentDeviceStateIs(const char *state)
{
    return TextEquals(CurrentDeviceState(), state);
}

bool CurrentGameStateIs(const char *state)
{
    return TextEquals(CurrentGameState(), state);
}

bool IsPlayerRole(const char *role)
{
    return TextEquals(role, "player");
}

bool IsTaggerRole(const char *role)
{
    return TextEquals(role, "tagger");
}

bool IsRevivalRole(const char *role)
{
    return TextEquals(role, "revival") || TextEquals(role, "ghost");
}

bool IsRevivalRole(const String &role)
{
    return IsRevivalRole(role.c_str());
}

bool IsFinalLifeTaken()
{
    if (my["life_chip"].isNull())
    {
        return false;
    }
    return my["life_chip"].as<int>() <= 1;
}

void StartPendingRevivalVibration()
{
    pending_revival_vibration = true;
}

void StopPendingRevivalVibration()
{
    pending_revival_vibration = false;
}

void SendRoleRequest(const char *role)
{
    has2wifi.Send((String)(const char *)my["device_name"], "role", role);
}

void ResetActivateSideEffects(bool clear_revival_help_records)
{
    motor_on = false;
    MotorStop();
    if (neopixel_timer.isEnabled(neopixel_timer_id))
    {
        neopixel_timer.deleteTimer(neopixel_timer_id);
    }
    ir_receive_timer.disable(ir_receive_timer_id);
    tag_capture_pending = false;
    hacking = false;
    StopPendingRevivalVibration();
    ResetRevivalTimer();
    if (clear_revival_help_records)
    {
        ClearRevivalHelpRecords();
    }
}

void ChangeLanguage()
{
    UpdateHmiLanguage();
}

void ResetRevivalTimer()
{
    revival = false;
    revival_timer_active = false;
    revival_finish_page = false;
    revival_finish_sent = false;
    revival_started_ms = 0;
    revival_bonus_ms = 0;
    revival_finish_started_ms = 0;
    last_role_send_ms = 0;
    last_revival_cooldown_count = 0;
}

void StartRevivalTimer()
{
    int revival_time = GameJsonInt("revival_time", DEFAULT_REVIVAL_TIME_SEC);
    if (revival_time <= 0)
    {
        revival_time = DEFAULT_REVIVAL_TIME_SEC;
    }

    revival = true;
    revival_timer_active = true;
    revival_finish_page = false;
    revival_finish_sent = false;
    revival_started_ms = millis();
    revival_bonus_ms = 0;
    revival_finish_started_ms = 0;
    last_role_send_ms = 0;
    last_revival_cooldown_count = GameJsonInt("revival_cooldown_count", 0);

    ir_receive_timer.disable(ir_receive_timer_id);
    lightColor(skyblue);
    sendCommand("sleep=0");
    UpdateHmiBattery();
    SendHmiValue("pgGhost.vReviveTimeMax", revival_time);
    SendHmiValue("pgGhost.vTick", 0);
    PageChange("pgGhost");
}

void UpdateRevivalTimer()
{
    if (!revival_timer_active || !IsRevivalRole(CurrentRole()))
    {
        return;
    }

    unsigned long revival_duration_ms = (unsigned long)GameJsonInt("revival_time", DEFAULT_REVIVAL_TIME_SEC) * 1000UL;
    if (revival_duration_ms == 0)
    {
        revival_duration_ms = (unsigned long)DEFAULT_REVIVAL_TIME_SEC * 1000UL;
    }

    unsigned long now = millis();
    unsigned long elapsed_ms = now - revival_started_ms + revival_bonus_ms;
    if (!revival_finish_page && elapsed_ms >= revival_duration_ms)
    {
        revival_finish_page = true;
        revival_finish_started_ms = now;
        UpdateHmiBattery();
        PageChange("pgRevival");
    }

    if (revival_finish_page && now - revival_finish_started_ms >= REVIVAL_FINISH_DISPLAY_MS)
    {
        if (!revival_finish_sent || now - last_role_send_ms >= ROLE_SEND_RETRY_MS)
        {
            SendRoleRequest("player");
            revival_finish_sent = true;
            last_role_send_ms = now;
        }
    }
}

void ApplyRevivalCooldownChange(int previous_count)
{
    int current_count = GameJsonInt("revival_cooldown_count", 0);
    if (!revival_timer_active)
    {
        last_revival_cooldown_count = current_count;
        return;
    }

    if (current_count < previous_count)
    {
        last_revival_cooldown_count = current_count;
        return;
    }

    int delta = current_count - previous_count;
    int reduce_time = GameJsonInt("revival_reduce_time", 0);
    if (delta > 0 && reduce_time > 0)
    {
        unsigned long bonus_ms = (unsigned long)delta * (unsigned long)reduce_time * 1000UL;
        revival_bonus_ms += bonus_ms;
        AddRevivalGaugeBonus(delta * reduce_time);
    }

    last_revival_cooldown_count = current_count;
}

void StartTagCapture()
{
    tag_capture_pending = true;
    tag_capture_started_ms = millis();
    PageChange("pgTagCapture");
}

void UpdateTagCaptureFlow()
{
    if (!tag_capture_pending)
    {
        return;
    }

    unsigned long elapsed_ms = millis() - tag_capture_started_ms;
    bool server_full = GameJsonInt("taken_chip", 0) >= GameJsonInt("max_taken_chip", 1);
    bool min_elapsed = elapsed_ms >= TAG_CAPTURE_MIN_MS;
    bool timeout_elapsed = elapsed_ms >= TAG_CAPTURE_TIMEOUT_MS;

    if ((server_full && min_elapsed) || timeout_elapsed)
    {
        tag_capture_pending = false;
        PageChange("pgTagFull");
    }
}

void ShowRolePage()
{
    const char *role = CurrentRole();
    sendCommand("sleep=0");

    if (IsPlayerRole(role))
    {
        StopPendingRevivalVibration();
        ResetRevivalTimer();
        hacking = false;
        lightColor(green);
        PageChange("pgSurvivor");
        irrecv.resume();
        ir_receive_timer.enable(ir_receive_timer_id);
        UpdateHmiBattery();
    }
    else if (IsTaggerRole(role))
    {
        StopPendingRevivalVibration();
        ResetRevivalTimer();
        hacking = false;
        lightColor(purple);
        ir_receive_timer.disable(ir_receive_timer_id);
        if (GameJsonInt("taken_chip", 0) >= GameJsonInt("max_taken_chip", 1))
        {
            PageChange("pgTagFull");
        }
        else
        {
            PageChange("pgTagEmpty");
        }
    }
    else if (IsRevivalRole(role))
    {
        StopPendingRevivalVibration();
        motor_on = false;
        MotorStop();
        hacking = false;
        if (!revival_timer_active)
        {
            StartRevivalTimer();
        }
        else
        {
            ir_receive_timer.disable(ir_receive_timer_id);
            lightColor(skyblue);
            sendCommand("sleep=0");
            if (!TextEquals(current_hmi_page, "pgGhost") && !TextEquals(current_hmi_page, "pgRevival"))
            {
                PageChange("pgGhost");
            }
        }
    }
}

void ShowResultPage()
{
    MotorStop();
    sendCommand("sleep=0");
    UpdateHmiResults();

    if (IsTaggerRole(CurrentRole()))
    {
        lightColor(purple);
        PageChange("pgTagResult");
    }
    else
    {
        lightColor(green);
        PageChange("pgSurResult");
    }
}

void HandleLifeChipChange()
{
    if (my["life_chip"].isNull())
    {
        return;
    }

    int life_chip = GameJsonInt("life_chip", -1);
    if (game_state != activate || !CurrentDeviceStateIs("activate") || !IsPlayerRole(CurrentRole()))
    {
        return;
    }

    if (life_chip <= 0)
    {
        StartPendingRevivalVibration();
        SendRoleRequest("revival");
        return;
    }

    hacking = false;
    hack_count = 0;
}

void HandleTakenChipChange(int previous_taken_chip)
{
    if (!IsTaggerRole(CurrentRole()) || !CurrentDeviceStateIs("activate"))
    {
        return;
    }

    int taken_chip = GameJsonInt("taken_chip", 0);
    if (taken_chip > previous_taken_chip)
    {
        StartTagCapture();
    }
    else if (taken_chip <= 0)
    {
        tag_capture_pending = false;
        PageChange("pgTagEmpty");
    }
}

void HandleResultFieldChange()
{
    UpdateHmiResults();
}

void HandleVibeChange()
{
    if (IsTaggerRole(CurrentRole()) || !CurrentGameStateIs("activate"))
    {
        return;
    }

    if ((int)my["vibe"] == 2)
    {
        motor_on = true;
    }
    else if ((int)my["vibe"] == 0)
    {
        motor_on = false;
        MotorStop();
    }
}

void SettingFunc()
{
    game_state = setting;
    MySerial1.print("setting ");

    ResetActivateSideEffects(true);
    lightColor(white);
    ledcWrite(BUZZER_PIN, 0);

    BatteryCheck();

    PageChange("0");
    sendCommand("sleep=1");
}

void ReadyFunc()
{
    game_state = ready;
    MySerial1.print("ready ");

    ResetActivateSideEffects(true);
    sendCommand("sleep=0");
    PageChange("pgInfo");
    lightColor(red);
}

void ActivateFunc()
{
    DisplayCheck();
    UpdateRevivalTimer();
    UpdateTagCaptureFlow();

    const char *role = CurrentRole();
    bool should_vibrate = motor_on || pending_revival_vibration;
    if (should_vibrate && IsPlayerRole(role))
    {
        MotorOn(vibration_pattern_2, ARRAYINDEX(vibration_pattern_2));
    }
    else
    {
        MotorStop();
    }

    if (CurrentDeviceStateIs("activate"))
    {
        if (IsTaggerRole(role) && GameJsonInt("taken_chip", 0) < GameJsonInt("max_taken_chip", 1))
        {
            IrSend();
        }
        else if (IsRevivalRole(role))
        {
            IrSend();
        }
    }
}

void ActivateRunOnce()
{
    game_state = activate;

    MySerial1.print("activate ");
    ledcWrite(BUZZER_PIN, 0);

    DisplaySet();
    ShowRolePage();
}

void HandleDeviceStateChange()
{
    const char *device_state = CurrentDeviceState();
    const char *role = CurrentRole();

    if (TextEquals(device_state, "activate"))
    {
        ShowRolePage();
    }
    else if (TextEquals(device_state, "player_win"))
    {
        MotorStop();
        if (IsTaggerRole(role))
        {
            PageChange("pgTagLose");
        }
        else
        {
            PageChange("pgSurWin");
        }
    }
    else if (TextEquals(device_state, "player_lose"))
    {
        MotorStop();
        if (IsTaggerRole(role))
        {
            PageChange("pgTagWin");
        }
        else
        {
            PageChange("pgSurLose");
        }
    }
    else if (TextEquals(device_state, "blink") && IsTaggerRole(role))
    {
        if (!neopixel_timer.isEnabled(neopixel_timer_id))
        {
            neopixel_timer_id = neopixel_timer.setInterval(400, tagger_blink);
        }
        sendCommand("sleep=0");
        PageChange("pgTagEmpty");
    }
    else if (TextEquals(device_state, "github"))
    {
        ota.check();
    }
    else if (TextEquals(device_state, "photo"))
    {
        ShowResultPage();
    }
}

void DataChange()
{
    static StaticJsonDocument<1000> cur;

    ChangeLanguage();

    if (my["brightness"].as<int>() != cur["brightness"].as<int>())
    {
        UpdateBrightness();
    }

    const char *my_game_state = CurrentGameState();
    const char *cur_game_state = cur["game_state"].as<const char *>();
    const char *my_device_state = CurrentDeviceState();
    const char *cur_device_state = cur["device_state"].as<const char *>();
    const char *my_role = CurrentRole();
    const char *cur_role = cur["role"].as<const char *>();

    if (!TextEquals(my_game_state, cur_game_state))
    {
        if (TextEquals(my_game_state, "setting"))
        {
            SettingFunc();
        }
        else if (TextEquals(my_game_state, "ready"))
        {
            ReadyFunc();
        }
        else if (TextEquals(my_game_state, "activate"))
        {
            ActivateRunOnce();
        }
    }

    if (!TextEquals(my_device_state, cur_device_state))
    {
        HandleDeviceStateChange();
    }

    if (!TextEquals(my_role, cur_role))
    {
        if (game_state == activate && !CurrentDeviceStateIs("photo"))
        {
            ShowRolePage();
        }
    }

    if (!my["life_chip"].isNull() && GameJsonInt("life_chip", -1) != cur["life_chip"].as<int>())
    {
        HandleLifeChipChange();
    }

    if (GameJsonInt("revival_cooldown_count", 0) != cur["revival_cooldown_count"].as<int>())
    {
        ApplyRevivalCooldownChange(cur["revival_cooldown_count"].as<int>());
    }

    if (GameJsonInt("battery_pack", 0) != cur["battery_pack"].as<int>())
    {
        UpdateHmiBattery();
    }

    if (GameJsonInt("taken_chip", 0) != cur["taken_chip"].as<int>())
    {
        HandleTakenChipChange(cur["taken_chip"].as<int>());
    }

    if (GameJsonInt("lv", 0) != cur["lv"].as<int>() ||
        GameJsonInt("lives_lost", 0) != cur["lives_lost"].as<int>() ||
        GameJsonInt("batteries_gained", 0) != cur["batteries_gained"].as<int>() ||
        GameJsonInt("round_taken_chip", 0) != cur["round_taken_chip"].as<int>())
    {
        HandleResultFieldChange();
    }

    if ((int)my["vibe"] != (int)cur["vibe"])
    {
        HandleVibeChange();
    }

    DebugPrintln("Data Change");

    cur = my;
}
