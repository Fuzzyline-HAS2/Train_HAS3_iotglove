#include "wifi_location.h"

bool TextEquals(const char *value, const char *expected)
{
  if (value == nullptr)
  {
    value = "";
  }
  if (expected == nullptr)
  {
    expected = "";
  }
  return strcmp(value, expected) == 0;
}

void HandleTtgoCommand(const String &command)
{
  if (command == "activate")
  {
    game_state = activate;
  }
  else if (command == "setting")
  {
    game_state = setting;
    stable_candidate_count = 0;
  }
  else if (command == "ready")
  {
    game_state = ready;
    stable_candidate_count = 0;
  }
  else if (IsGithubCommand(command))
  {
    game_state = ready;
    stable_candidate_count = 0;
    RunBeetleOta(command);
  }
}

void ReadTtgoCommands()
{
  while (MySerial1.available() > 0)
  {
    String command = MySerial1.readStringUntil(' ');
    command.trim();
    if (command.length() > 0)
    {
      Serial.print("[TTGO] ");
      Serial.println(command);
      HandleTtgoCommand(command);
    }
  }
}
