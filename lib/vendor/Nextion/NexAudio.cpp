/**
 * @file NexAudio.cpp
 * @author Kim YuBin
 * @brief 
 * @version 0.1
 * @date 2022-10-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "NexAudio.h"

NexAudio::NexAudio(uint8_t pid, uint8_t cid, const char *name)
    :NexTouch(pid, cid, name)
{
}

bool NexAudio::getVid(uint32_t *number)
{
    String cmd = String("get ");
    cmd += getObjName();
    cmd += ".vid=";
    sendCommand(cmd.c_str());
    return recvRetNumber(number);
}

bool NexAudio::setVid(uint32_t number)
{
    char buf[10] = {0};
    String cmd;

    utoa(number, buf, 10);
    cmd += getObjName();
    cmd += ".vid=";
    cmd += buf;
    sendCommand(cmd.c_str());
    return recvRetCommandFinished();
}

bool NexAudio::getEn(uint32_t *number)
{
    String cmd = String("get ");
    cmd += getObjName();
    cmd += ".en=";
    sendCommand(cmd.c_str());
    return recvRetNumber(number);
}

bool NexAudio::setEn(uint32_t number)
{
    char buf[10] = {0};
    String cmd;

    utoa(number, buf, 10);
    cmd += getObjName();
    cmd += ".en=";
    cmd += buf;
    sendCommand(cmd.c_str());
    return recvRetCommandFinished();
}
