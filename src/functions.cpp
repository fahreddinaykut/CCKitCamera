#include "functions.h"

functions::functions()
{
}
void functions::init(variables *VARS)
{
  vars = VARS;
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  if (!EEPROM.begin(512))
  {
    Serial.println("failed to init EEPROM");
  }
  if (functions::loadLedStatus() == 1)
  {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
    functions::loadFromEEPROM(); // Read values from eeprom
  functions::writeUpdateMode(0);
}
void functions::loadFromEEPROM()
{
  for (int i = 0; i < 32; ++i)
  {
    byte readValue = EEPROM.read(i);

    if (readValue == '|')
    {
      break;
    }

    vars->esid += char(readValue);
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(vars->esid);
  Serial.println("Reading EEPROM pass");
  for (int i = 32; i < 96; ++i)
  {
    byte readValue = EEPROM.read(i);

    if (readValue == '|')
    {
      break;
    }

    vars->epass += char(readValue);
  }
  Serial.print("PASS: ");
  Serial.println(vars->epass);
}
void functions::saveToEEPROM(String qsid, String qpass)
{
  qsid.concat("|");
  qpass.concat("|");
  Serial.println("writing eeprom ssid:");
  for (int i = 0; i < qsid.length(); ++i)
  {
    EEPROM.write(i, qsid[i]);
    Serial.print("Wrote: ");
    Serial.println(qsid[i]);
  }
  Serial.println("writing eeprom pass:");
  for (int i = 0; i < qpass.length(); ++i)
  {
    EEPROM.write(32 + i, qpass[i]);
    Serial.print("Wrote: ");
    Serial.println(qpass[i]);
  }
  EEPROM.commit();
}

void functions::writeLedStatus(uint8_t mode)
{
  EEPROM.write(510, mode); // EEPROM.put(address, boardId);
  EEPROM.commit();
}
uint8_t functions::loadLedStatus()
{
  return EEPROM.read(510);
}

uint8_t functions::loadUpdateMode()
{
  return EEPROM.read(511);
}
void functions::writeUpdateMode(uint8_t mode)
{
  EEPROM.write(511, mode); // EEPROM.put(address, boardId);
  EEPROM.commit();
}
uint8_t functions::loadWifiMode()
{
  return EEPROM.read(509);
}
void functions::writeWifiMode(uint8_t mode)
{
  EEPROM.write(509, mode); // EEPROM.put(address, boardId);
  EEPROM.commit();
}