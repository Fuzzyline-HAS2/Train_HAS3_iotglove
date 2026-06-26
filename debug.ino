#include "Train_HAS3_iotglove.h"

#ifdef __DEBUG__
WiFiServer debug_telnet_server(DEBUG_TELNET_PORT);
WiFiClient debug_telnet_client;
bool debug_telnet_started = false;

class DebugTelnetPrint : public Print
{
public:
  size_t write(uint8_t value) override
  {
    if (debug_telnet_client && debug_telnet_client.connected())
    {
      debug_telnet_client.write(value);
    }
    return 1;
  }

  size_t write(const uint8_t *buffer, size_t size) override
  {
    if (debug_telnet_client && debug_telnet_client.connected())
    {
      debug_telnet_client.write(buffer, size);
    }
    return size;
  }
};

DebugTelnetPrint debug_telnet_print;

Print *DebugOutput()
{
  return &debug_telnet_print;
}

void DebugInit()
{
  if (debug_telnet_started)
  {
    return;
  }

  debug_telnet_server.begin();
  debug_telnet_server.setNoDelay(true);
  debug_telnet_started = true;
}

void DebugPoll()
{
  if (!debug_telnet_started)
  {
    return;
  }

  WiFiClient new_client = debug_telnet_server.available();
  if (new_client)
  {
    if (debug_telnet_client && debug_telnet_client.connected())
    {
      debug_telnet_client.stop();
    }

    debug_telnet_client = new_client;
    debug_telnet_client.setNoDelay(true);
    debug_telnet_client.println("IoTGlove debug connected");
  }

  if (debug_telnet_client && !debug_telnet_client.connected())
  {
    debug_telnet_client.stop();
  }
}

bool DebugConnected()
{
  return debug_telnet_client && debug_telnet_client.connected();
}

void DebugPrint(const char *value)
{
  if (DebugConnected())
  {
    debug_telnet_client.print(value);
  }
}

void DebugPrint(const String &value)
{
  if (DebugConnected())
  {
    debug_telnet_client.print(value);
  }
}

void DebugPrint(unsigned long value)
{
  if (DebugConnected())
  {
    debug_telnet_client.print(value);
  }
}

void DebugPrintln()
{
  if (DebugConnected())
  {
    debug_telnet_client.println();
  }
}

void DebugPrintln(const char *value)
{
  if (DebugConnected())
  {
    debug_telnet_client.println(value);
  }
}

void DebugPrintln(const String &value)
{
  if (DebugConnected())
  {
    debug_telnet_client.println(value);
  }
}

void DebugPrintln(unsigned long value)
{
  if (DebugConnected())
  {
    debug_telnet_client.println(value);
  }
}

void DebugPrintln(unsigned long value, int base)
{
  if (DebugConnected())
  {
    debug_telnet_client.println(value, base);
  }
}

void DebugPrintf(const char *format, ...)
{
  if (!DebugConnected())
  {
    return;
  }

  char buffer[160];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  debug_telnet_client.print(buffer);
}

#else
Print *DebugOutput() { return &Serial; }
void DebugInit() {}
void DebugPoll() {}
void DebugPrint(const char *value) {}
void DebugPrint(const String &value) {}
void DebugPrint(unsigned long value) {}
void DebugPrintln() {}
void DebugPrintln(const char *value) {}
void DebugPrintln(const String &value) {}
void DebugPrintln(unsigned long value) {}
void DebugPrintln(unsigned long value, int base) {}
void DebugPrintf(const char *format, ...) {}
#endif
