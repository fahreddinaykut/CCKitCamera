
#include "constants.h"
#include <WiFi.h>
#include "cameraServer.h"
nowLib now;
variables vars;
#define SDA 14
#define SCL 15
uint32_t notConnectedCounter = 0;
void setup()
{
  Serial.begin(115200);
  
  Serial.println("CCKIT CAM Started");
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Wire.begin(14, 15);
  funs.init(&vars); // BEGIN EEPROM
  now.init(&vars,&funs,&now);
  updatemode = funs.loadUpdateMode();
  xTaskCreate(
      i2cTask,                  /* Function to implement the task */
      "Task1",                  /* Name of the task */
      10000,                    /* Stack size in words */
      NULL,                     /* Task input parameter */
      1,                        /* Priority of the task */
      NULL /* Task handle. */); /* Core where the task should run */

  if (updatemode)
  {
    server.on("/", HTTP_GETconflict, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Go to /update"); });

    AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    Serial.println("Update Mode Started");
    funs.writeUpdateMode(0);
  }
  else
  {
    initCamera(vars.esid,vars.epass,funs.loadWifiMode());
  }
}

void loop()
{
  now.broadcast();
  // now.send("pulse", 1);
  sendDebugMessages(1000);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}
void i2cTask(void *parameter)
{
  if (!sht31.begin(0x44))
  { // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1)
      vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  for (;;)
  {
    measure();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
void measure()
{
  byte error;
  temp = sht31.readTemperature();
  hum = sht31.readHumidity();
  if (!isnan(temp) && !isnan(hum))
  {
    twoByte temptwo;
    twoByte humtwo;
    temptwo.iVal = temp * 10.0;
    humtwo.iVal = hum * 10.0;
    uint8_t readData[] = {cmdTemp, temptwo.bVal[0], temptwo.bVal[1], humtwo.bVal[0], humtwo.bVal[1]};
    std::string rdVal = std::string((char *)readData, 5);
    now.send(rdVal, rdVal.length());
    error = 0;
  }
  else
  {
    Serial.println("Failed to read humidity");
    error = 1;
  }
  uint8_t errorData[] = {cmdCamTempError, error};
  std::string errorDataS = std::string((char *)errorData, 2);
  now.send(errorDataS, errorDataS.length());
}

void sendDebugMessages(int tick)
{
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  if (currentMillis - previousMillis >= tick)
  {
    Serial.printf("Temp:%.2f\t Humudity:%.2f\t \n", temp, hum);
    previousMillis = currentMillis;
  }
}
