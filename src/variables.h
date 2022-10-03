#pragma once
#include "Arduino.h"
#define cmdTemp 0x01
#define cmdHum 0x02
#define cmdResponseSetting 0x04
#define cmdFlashToggle 0x5
#define cmdCamTempError 0x6
#define cmdCamUpdateMode 0x7

#define LED_BUILTIN 4
class variables {
private:
public:
uint8_t cckitMainStatus=0;
String esid;
String epass = "";
char cckitMainMac[18]={0};
   uint8_t cckitMainMacBytes[6] ;
};