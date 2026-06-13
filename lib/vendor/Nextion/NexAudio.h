/**
 * @file NexAudio.h
 * @author Kim YuBin
 * @brief 
 * @version 0.1
 * @date 2022-10-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __NEXAUDIO_H__
#define __NEXAUDIO_H__

#include "NexTouch.h"
#include "NexHardware.h"

class NexAudio: public NexTouch
{
    public: /* methods */
    NexAudio(uint8_t pid, uint8_t cid, const char *name);

    bool getVid(uint32_t *number);

    bool setVid(uint32_t number);

    bool getEn(uint32_t *number);
    
    bool setEn(uint32_t number);
};

#endif