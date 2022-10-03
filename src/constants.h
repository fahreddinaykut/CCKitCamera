#pragma once
#include "Arduino.h"
#include "esp_camera.h"

#include "esp_timer.h"
#include "img_converters.h"

#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems
#include "esp_http_server.h"
#include "Adafruit_BusIO_Register.h"
#include <Wire.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "AsyncElegantOTA.h"
#include "functions.h"
#include "variables.h"
#include "nowLib.h"
#include "Adafruit_SHT31.h"
#define CAMERA_MODEL_AI_THINKER
// #define CAMERA_MODEL_WROVER_KIT
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#define EEPROM_SIZE 64
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
AsyncWebServer server(80);

union twoByte
{
  byte bVal[2];
  int iVal;
};
int addr = 0;
uint8_t updatemode = 0;
void startDataServer();
void initCam();
void sendDebugMessages(int tick);
void measure();

float temp = 0.0f;
float hum = 0.0f;
void i2cTask(void *parameter);

