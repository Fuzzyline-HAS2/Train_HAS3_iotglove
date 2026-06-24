#include "Train_HAS3_iotglove.h"

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
    // vVersion은 글로벌 변수라 어느 페이지에서든 읽힘.
    // 페이지를 이동(page pgSetting -> page 0)하면 게임 화면이 검은 page 0으로
    // 튀므로 이동 없이 직접 읽기만 수행한다. 실패 시 UpdateVersionReport가 재시도.
    return ReadHmiNumber("pgSetting.vVersion.val", version, 500);
}

void DisplaySet()
{
    UpdateHmiLanguage();
    UpdateHmiBattery();
    UpdateHmiResults();
    SendHmiValue("pgGhost.vReviveTimeMax", JsonIntOrDefault("revival_time", DEFAULT_REVIVAL_TIME_SEC));
}

bool ReadNextionPageName(char *buf, uint8_t buf_len)
{
    static char rx[32];
    static uint8_t rx_len = 0;

    while (nexSerial.available() > 0)
    {
        char c = (char)nexSerial.read();
        if (c == '\r') continue;
        if (c == '\n')
        {
            rx[rx_len] = '\0';
            if (rx_len > 0)
            {
                strncpy(buf, rx, buf_len - 1);
                buf[buf_len - 1] = '\0';
                rx_len = 0;
                return true;
            }
            rx_len = 0;
            continue;
        }
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
        {
            if (rx_len < sizeof(rx) - 1)
                rx[rx_len++] = c;
        }
        else
        {
            rx_len = 0;
        }
    }
    return false;
}

void ApplyNeoPixelForPage(const char *page)
{
    if      (strcmp(page, "pgStart")    == 0) lightColor(white);
    else if (strcmp(page, "pgInfo")     == 0) lightColor(red);
    else if (strcmp(page, "pgSurvivor") == 0) lightColor(green);
    else if (strcmp(page, "pgGhost")    == 0) lightColor(blue);
    else if (strcmp(page, "pgAltarTag") == 0) lightColor(purple);
    else if (strcmp(page, "pgTagEmpty") == 0) lightColor(purple);
}

void DisplayCheck()
{
    char page[32];
    if (ReadNextionPageName(page, sizeof(page)))
    {
        snprintf(current_hmi_page, sizeof(current_hmi_page), "%s", page);
    }
}

void PageChange(const char *page)
{
    bool page_changed = !TextEquals(current_hmi_page, page);
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "page %s", page);
    sendCommand(cmd);
    ApplyNeoPixelForPage(page);
    if (page_changed)
    {
        StartPageChangeVibration();
    }
    snprintf(current_hmi_page, sizeof(current_hmi_page), "%s", page);
}
