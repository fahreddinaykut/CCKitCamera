
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems
#include "esp_http_server.h"
#include <esp_now.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Org_01.h>
#include <uFire_SHT3x.h>
#include "myfonts.h"
#include "EEPROM.h"
#include "image.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "AsyncElegantOTA.h"

#define cmdTemp 0x01
#define cmdHum 0x02
#define cmdCamStatus 0x03
#define cmdResponseSetting 0x04
#define cmdFlashToggle 0x5
#define cmdCamTempError 0x6
#define cmdCamUpdateMode 0x7

#define EEPROM_SIZE 64
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define CHANNEL 1
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0
#define LED_BUILTIN 4
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
    AsyncWebServer server(80);
typedef struct broadcast_message
{
  char type[16] = "unknown"; // data or broadcast
  char ssid[16] = "slave";   // slave or master
  char pass[16] = "unknown"; // slider pan tilt
  uint8_t datalen = 1;
  char data[128] = " ";
} broadcast_message;
union twoByte
{
  byte bVal[2];
  int iVal;
};
uFire::SHT3x sht30;
int addr = 0;
uint8_t updatemode = 0;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void startDataServer();
void initCam();
void drawCentreString(const char *buf, int x, int y);
void sendData(std::string incomingdata, uint8_t len);
void initBroadcastSlave();
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void saveToEEPROM(String qsid, String qpass);
void loadFromEEPROM();
void broadcast();
void sendDebugMessages(int tick);
void processData(std::string value);
void writeLedStatus(uint8_t mode);
void writeUpdateMode(uint8_t mode);
uint8_t loadUpdateMode();
uint8_t loadLedStatus();
const char *ssidAP = "CCKIT";
const char *passwordAP = "123456789";

float temp = 0.0f;
float hum = 0.0f;
void displayTask(void *parameter);

IPAddress staticIP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 254);

broadcast_message myData;
byte nowInit;
esp_now_peer_info_t slave;
byte peerConnected;

void measure()
{
  uint8_t errorData1[] = {cmdCamTempError, 0x1};
  uint8_t errorData2[] = {cmdCamTempError, 0x2};
  std::string errorData1S = std::string((char *)errorData1, 2);
  std::string errorData2S = std::string((char *)errorData2, 2);
  sht30.measure();
  switch (sht30.status)
  {
  case sht30.STATUS_NOT_CONNECTED:
    sendData(errorData1S, errorData1S.length()); //Send error if cannot read sensor
    break;
  case sht30.STATUS_CRC_ERROR:
    sendData(errorData2S, errorData1S.length());//Send error if cannot read sensor
    break;
  case sht30.STATUS_NO_ERROR:
    temp = sht30.tempC;
    hum = sht30.RH;
    twoByte temptwo;
    twoByte humtwo;
    temptwo.iVal = temp * 10.0;
    humtwo.iVal = hum * 10.0;
    Serial.printf("temp %d hum %d\n", temptwo.iVal, humtwo.iVal);
    uint8_t tempData[] = {cmdTemp, temptwo.bVal[0], temptwo.bVal[1]};
    std::string tempDataval = std::string((char *)tempData, 3);
    sendData(tempDataval, 3);
    uint8_t humData[] = {cmdHum, humtwo.bVal[0], humtwo.bVal[1]};
    std::string humDataval = std::string((char *)humData, 3);
    sendData(humDataval, 3);
    // Serial.println((String)sht30.tempC + " °C");
    // Serial.println((String)sht30.RH + " %RH");
    // Serial.println((String)sht30.vpd_kPa + " VPD kPa");
    // Serial.println((String)sht30.dew_pointC + " dew point °C");
    break;
  }
}
// 24:62:AB:D0:C7:90
// AC:67:B2:C8:1E:38
uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0xC8, 0x1E, 0x38};

// Replace with your network credentials
String esid;
String epass = "";

#define PART_BOUNDARY "123456789000000000000987654321"

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
String wifiurl = "NO WIFI";
static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
  {
    return res;
  }

  while (true)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->width > 400)
      {
        if (fb->format != PIXFORMAT_JPEG)
        {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted)
          {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        }
        else
        {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK)
    {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    }
    else if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK)
    {
      break;
    }
    // Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  // Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(stream_httpd, &index_uri);
  }
}
void setup()
{

  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  if (!EEPROM.begin(512))
  {
    Serial.println("failed to init EEPROM");
  }
  // saveToEEPROM("Aykut","edirne12345");
  if (loadLedStatus() == 1)
  {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  loadFromEEPROM();
 updatemode=  loadUpdateMode();
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Wire.begin(2, 13);
  sht30.begin();
  xTaskCreate(
      displayTask,              /* Function to implement the task */
      "Task1",                  /* Name of the task */
      10000,                    /* Stack size in words */
      NULL,                     /* Task input parameter */
      1,                        /* Priority of the task */
      NULL /* Task handle. */); /* Core where the task should run */
    // Wi-Fi connection
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    WiFi.config(staticIP, gateway, subnet, dns, dns);
    WiFi.begin(esid.c_str(), epass.c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.print("Camera Stream Ready! Go to: http://");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.macAddress());
    wifiurl = WiFi.localIP().toString();

    if (esp_now_init() == ESP_OK)
    {
      Serial.println("ESPNow Init Success");
      nowInit = 1;
      esp_now_register_send_cb(OnDataSent);
      esp_now_register_recv_cb(OnDataRecv);
      initBroadcastSlave();
    }
    else
    {
      nowInit = 0;
      Serial.println("ESPNow Init Failed");
    }
  if (updatemode)
  {



    server.on("/", HTTP_GETconflict, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Go to /update"); });

    AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    Serial.println("Update Mode Started");
    writeUpdateMode(0);
  }
  else

  {
    Serial.println("Normal mode");
    initCam();
  }
}

void loop()
{
  // ((String) "broadcast").toCharArray(myData.data, 16);
  // uint8_t     sendData2[] = { 0x56, 0X00, 0X00, 0x00 };
  //         std::string val         = std::string((char*)sendData2, 4);
  // for (int i = 0; i < val.length(); i++) {
  //     myData.data[i] = val[i];
  // }
  broadcast();
  sendData("pulse", 1);
  sendDebugMessages(1000);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}
void displayTask(void *parameter)
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.clearDisplay();
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(WHITE);
  display.setTextSize(1); // Draw 2X-scale text
  drawCentreString("CCKIT CAM", 64, 24);
  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  for (;;)
  {
    measure();
    display.clearDisplay();
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    drawCentreString(wifiurl.c_str(), 64, 29);
    display.setFont(&SourceSansPro_Regular6pt7b);
    display.setTextSize(1);
    display.setCursor(80, 10);
    display.print((int)temp);
    display.print("C° ");
    display.print((int)hum);
    display.println("% ");
    drawCentreString(esid.c_str(), 38, 10);
    if (WiFi.isConnected())
    {
      display.drawBitmap(4, 0, wifi1_icon16x16, 16, 16, 1);
      uint8_t wifiData[] = {cmdCamStatus, 0x1};
      std::string wifiDataval = std::string((char *)wifiData, 2);
      sendData(wifiDataval, 2);
    }
    else
    {
      display.drawBitmap(4, 0, cancel_icon16x16, 16, 16, 1);
      uint8_t wifiData[] = {cmdCamStatus, 0x0};
      std::string wifiDataval = std::string((char *)wifiData, 2);
      sendData(wifiDataval, 2);
    }
    display.display();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void drawCentreString(const char *buf, int x, int y)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(buf, 0, y, &x1, &y1, &w, &h); // calc width of new string
  display.setCursor(x - w / 2, y);
  display.print(buf);
}
void initBroadcastSlave()
{
  // clear slave data
  memset(&slave, 0, sizeof(slave));
  for (int ii = 0; ii < 6; ++ii)
  {
    slave.peer_addr[ii] = (uint8_t)0xff;
  }
  esp_err_t addStatus = esp_now_add_peer(&slave);
}

// send data
void sendData(std::string incomingdata, uint8_t len)
{
  broadcast_message data;
  data.datalen = len;

  ((String) "data").toCharArray(data.type, 16);
  for (int i = 0; i < incomingdata.length(); i++)
  {
    data.data[i] = incomingdata[i];
  }
  esp_err_t result2 = esp_now_send(0, (uint8_t *)&data, sizeof(data)); // send to all peers
}

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)
  {
  }
  else
  {
    esp_err_t delStatus = esp_now_del_peer(mac_addr);
    peerConnected = 0;
    Serial.println("peer disconnected");
  }
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  broadcast_message myData;
  memcpy(&myData, data, sizeof(myData));
  esp_now_peer_info_t mypeerInf;
  mypeerInf.channel = CHANNEL;
  mypeerInf.encrypt = 0;
  mypeerInf.ifidx = WIFI_IF_STA;
  memcpy(mypeerInf.peer_addr, mac_addr, 6);

  if ((String)myData.type == "broadcast")
  {
    if (!esp_now_is_peer_exist(mac_addr))
      if (esp_now_add_peer(&mypeerInf) != ESP_OK)
      {
        Serial.println("peer couldn't added");
        return;
      }
      else
      {
        peerConnected = 1;
        Serial.println("peer connected");
      }
  }
  else if ((String)myData.type == "data")
  {
    //   grbl_sendf(CLIENT_ALL, "MAC:%s Data:%s\n", macStr, myData.data);
    int i;
    std::string s = "";
    for (i = 0; i < myData.datalen; i++)
    {
      s = s + myData.data[i];
    }
    processData(s);
  }
  else if ((String)myData.type == "wificonfig")
  {
    ssidAP = myData.ssid;
    passwordAP = myData.pass;
    saveToEEPROM(ssidAP, passwordAP);
    uint8_t data[] = {cmdResponseSetting, 0x1};
    std::string sendDataVal = std::string((char *)data, 2);
    sendData(sendDataVal, sendDataVal.length());
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP.restart();
  }
}

void initCam()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // YUV422,GRAYSCALE,RGB565,JPEG
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_HVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  else
  {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    // return;
  }
  else
  {

    startCameraServer();
  }
}

void loadFromEEPROM()
{
  for (int i = 0; i < 32; ++i)
  {
    byte readValue = EEPROM.read(i);

    if (readValue == '|')
    {
      break;
    }

    esid += char(readValue);
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  for (int i = 32; i < 96; ++i)
  {
    byte readValue = EEPROM.read(i);

    if (readValue == '|')
    {
      break;
    }

    epass += char(readValue);
  }
  Serial.print("PASS: ");
  Serial.println(epass);
}
void saveToEEPROM(String qsid, String qpass)
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
void processData(std::string value)
{
  switch ((byte)value[0])
  {
  case cmdFlashToggle:
    if (digitalRead(LED_BUILTIN) == HIGH)
    {
      digitalWrite(LED_BUILTIN, LOW);
      writeLedStatus(0);
    }
    else
    {
      digitalWrite(LED_BUILTIN, HIGH);
      writeLedStatus(1);
    }

    break;
    case cmdCamUpdateMode:
    writeUpdateMode(1);
    ESP.restart();
    break;
  default:
    break;
  }
}
void broadcast()
{
  esp_now_peer_info_t slave;
  slave.channel = CHANNEL; // pick a channel
  slave.encrypt = 0;       // no encryption
  for (int ii = 0; ii < 6; ++ii)
  {
    slave.peer_addr[ii] = (uint8_t)0xff;
  }
  broadcast_message data;
  ((String) "broadcast").toCharArray(data.type, 16);
  esp_err_t result = esp_now_send(slave.peer_addr, (uint8_t *)&data, sizeof(data));
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
void writeLedStatus(uint8_t mode)
{
  EEPROM.write(510, mode); // EEPROM.put(address, boardId);
  EEPROM.commit();
}
uint8_t loadLedStatus()
{
  return EEPROM.read(510);
}
uint8_t loadUpdateMode()
{
  return EEPROM.read(511);
}
void writeUpdateMode(uint8_t mode)
{
  EEPROM.write(511, mode); // EEPROM.put(address, boardId);
  EEPROM.commit();
}