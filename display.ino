#include "updated_IoTglove.h"

int ClampDisplayValue(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

int JsonIntOrDefault(const char *key, int fallback)
{
    if (my[key].isNull())
    {
        return fallback;
    }
    return my[key].as<int>();
}

void SendHmiValue(const char *target, int value)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%s.val=%d", target, value);
    sendCommand(cmd);
}

void SendHmiPic(const char *page, const char *component, const char *base_var, int value, int min_value, int max_value)
{
    int clamped = ClampDisplayValue(value, min_value, max_value);
    char cmd[96];
    snprintf(cmd, sizeof(cmd), "%s.%s.pic=%s.%s.val+%d", page, component, page, base_var, clamped);
    sendCommand(cmd);
}

void UpdatePageBattery(const char *page)
{
    int battery_pack = ClampDisplayValue(JsonIntOrDefault("battery_pack", 0), 0, 4);
    char target[40];
    snprintf(target, sizeof(target), "%s.vBatteryPack", page);
    SendHmiValue(target, battery_pack);
    SendHmiPic(page, "pBatteryPack", "vBatPicNum", battery_pack, 0, 4);
}

void UpdateHmiLanguage()
{
    const char *selected_language = shift_machine["selected_language"].as<const char *>();
    int hmi_language = (selected_language != NULL && strcmp(selected_language, "EN") == 0) ? 0 : 1;
    SendHmiValue("pgInfo.vLanguage", hmi_language);
}

void UpdateHmiBattery()
{
    UpdatePageBattery("pgSurvivor");
    UpdatePageBattery("pgGhost");
    UpdatePageBattery("pgRevival");
}

void UpdateHmiResults()
{
    int level = JsonIntOrDefault("lv", 0);
    int lives_lost = JsonIntOrDefault("lives_lost", 0);
    int batteries_gained = JsonIntOrDefault("batteries_gained", 0);
    int round_taken_chip = JsonIntOrDefault("round_taken_chip", 0);

    SendHmiValue("pgSurResult.vSurLevel", ClampDisplayValue(level, 0, 99));
    SendHmiValue("pgSurResult.vSurTakenLives", ClampDisplayValue(lives_lost, 0, 30));
    SendHmiValue("pgSurResult.vBatTotal", ClampDisplayValue(batteries_gained, 0, 20));
    SendHmiValue("pgTagResult.vTagLevel", ClampDisplayValue(level, 0, 99));
    SendHmiValue("pgTagResult.vTagTakenLives", ClampDisplayValue(round_taken_chip, 0, 30));
}

void AddRevivalGaugeBonus(int seconds)
{
    if (seconds <= 0)
    {
        return;
    }

    // HMI(pgGhost)의 게이지 타이머 tScreenRefresh는 tim=1000ms마다 vTick을 +1 한다(초당 1 tick).
    //   완료 시점: vTick >= vReviveTimeMax * (1000 / tim)
    // 도움받은 seconds만큼 막대를 앞으로 당기려면 (seconds * 1000/tim) tick을 더한다.
    // 1000/tim 은 REVIVAL_TICK_PER_SEC 상수로 박아 둔다(tim 변경 시 함께 갱신).
    int bonus_ticks = seconds * REVIVAL_TICK_PER_SEC;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "pgGhost.vTick.val=pgGhost.vTick.val+%d", bonus_ticks);
    sendCommand(cmd);
}

bool ReadHmiNumber(const char *target, uint32_t *value, uint32_t timeout_ms)
{
    if (target == NULL || value == NULL)
    {
        return false;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "get %s", target);
    sendCommand(cmd);
    return recvRetNumber(value, timeout_ms);
}

bool ReadNextionVersion(uint32_t *version)
{
    if (ReadHmiNumber("pgSetting.vVersion.val", version, 300))
    {
        return true;
    }

    sendCommand("page pgSetting");
    delay(150);
    bool ok = ReadHmiNumber("pgSetting.vVersion.val", version, 500);
    sendCommand("page 0");
    delay(50);
    return ok;
}

void DisplaySet()
{
    UpdateHmiLanguage();
    UpdateHmiBattery();
    UpdateHmiResults();
    SendHmiValue("pgGhost.vReviveTimeMax", JsonIntOrDefault("revival_time", DEFAULT_REVIVAL_TIME_SEC));
}

void DisplayCheck()
{
    while (MySerial2.available() > 0)
    {
        MySerial2.read();
    }
}

void PageChange(const char *page)
{
    bool page_changed = !TextEquals(current_hmi_page, page);
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "page %s", page);
    sendCommand(cmd);
    if (page_changed)
    {
        StartPageChangeVibration();
    }
    snprintf(current_hmi_page, sizeof(current_hmi_page), "%s", page);
}
