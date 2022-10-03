#pragma once
#include "Arduino.h"
#include "EEPROM.h"
#include "variables.h"

class functions{
    private:
    public:
    variables *vars;
    functions();
    void init(variables *VARS);
    void loadFromEEPROM();
    void saveToEEPROM(String qsid, String qpass);
    void writeUpdateMode(uint8_t mode);
uint8_t loadUpdateMode();
uint8_t loadLedStatus();
void writeLedStatus(uint8_t mode);
uint8_t loadWifiMode();
void writeWifiMode(uint8_t mode);
};