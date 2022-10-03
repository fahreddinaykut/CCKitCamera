#include "nowLib.h"
nowLib::nowLib()
{
}
void nowLib::init(variables *VARS, functions *FUNS, nowLib *NOW)
{
  vars = VARS;
  svars = vars;
  now = NOW;
  snow = now;
  funs = FUNS;
  sfuns = funs;

  esp_now_peer_info_t slave;
  WiFi.mode(WIFI_AP_STA);

  // int a = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);

  esp_now_deinit();
  esp_now_init();
  esp_now_register_send_cb(OnDataSent);

  esp_now_register_recv_cb(OnDataRecv);
  // clear slave data
  memset(&slave, 0, sizeof(slave));
  for (int ii = 0; ii < 6; ++ii)
  {
    slave.peer_addr[ii] = (uint8_t)0xff;
  }
  slave.channel = CHANNEL; // pick a channel
  slave.encrypt = 0;       // no encryption
  const esp_now_peer_info_t *peer = &slave;
  esp_now_add_peer(peer); // add FF broadcast peer
  initialized = 1;
}
void nowLib::addBroadcastPeer()
{
  // clear slave data
  memset(&slave, 0, sizeof(slave));
  slave.channel = CHANNEL;
  slave.encrypt = 0;
  slave.ifidx = WIFI_IF_STA;
  for (int ii = 0; ii < 6; ++ii)
  {
    slave.peer_addr[ii] = (uint8_t)0xff;
  }
  esp_err_t addStatus = esp_now_add_peer(&slave);
}
void nowLib::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  char macStr[18] PROGMEM = {0};
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  if ((String)macStr == (String)svars->cckitMainMac)
  {
    if (status == ESP_NOW_SEND_SUCCESS)
    {
      if ((String)macStr == (String)svars->cckitMainMac)
      {
        svars->cckitMainStatus = 1;
      }
    }
    else
    {
      esp_err_t delStatus = esp_now_del_peer(mac_addr);
      svars->cckitMainStatus = 0;
      Serial.println("peer disconnected");
    }
  }
}
void nowLib::OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  char macStr[18] PROGMEM = {0};
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  broadcast_message myData;
  memcpy(&myData, data, sizeof(myData));
  esp_now_peer_info_t mypeerInf;
  mypeerInf.channel = CHANNEL;
  mypeerInf.encrypt = 0;
  mypeerInf.ifidx = WIFI_IF_STA;
  memcpy(mypeerInf.peer_addr, mac_addr, 6);

  if ((String)myData.type == "broadcast")
  {
    if ((String)myData.device == "main")
    {

      if (!esp_now_is_peer_exist(mac_addr))
      {
        if (esp_now_add_peer(&mypeerInf) != ESP_OK)
        {
          Serial.println("peer couldn't added");
          return;
        }
        else
        {
          svars->cckitMainStatus = 1;
          Serial.println("Main Connected");
          sprintf(svars->cckitMainMac, // save macadress in variables library
                  "%02X:%02X:%02X:%02X:%02X:%02X",
                  mac_addr[0],
                  mac_addr[1],
                  mac_addr[2],
                  mac_addr[3],
                  mac_addr[4],
                  mac_addr[5]);
          memcpy(svars->cckitMainMacBytes, mac_addr, 6);
        }
      }
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
    snow->processData(s);
  }
  else if ((String)myData.type == "wificonfig")
  {
    Serial.println("wifi config received");
    sfuns->saveToEEPROM(myData.ssid, myData.pass);
    uint8_t data[] = {cmdResponseSetting, 0x1};
    std::string sendDataVal = std::string((char *)data, 2);
    snow->send(sendDataVal, sendDataVal.length());
    Serial.println("Wifi setted from main module.");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP.restart();
  }
}
void nowLib::send(std::string incomingdata, uint8_t len)
{
  broadcast_message data;
  data.datalen = len;
  ((String) "camera").toCharArray(data.device, 16);
  ((String) "data").toCharArray(data.type, 16);
  for (int i = 0; i < incomingdata.length(); i++)
  {
    data.data[i] = incomingdata[i];
  }
  esp_err_t result2 = esp_now_send(0, (uint8_t *)&data, sizeof(data)); // send to all peers
}
void nowLib::broadcast()
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
  ((String) "camera").toCharArray(data.device, 16);
  esp_err_t result = esp_now_send(slave.peer_addr, (uint8_t *)&data, sizeof(data));
}
void nowLib::processData(std::string value)
{
  switch ((byte)value[0])
  {
  case cmdFlashToggle:
    if (digitalRead(LED_BUILTIN2) == HIGH)
    {
      digitalWrite(LED_BUILTIN2, LOW);
      funs->writeLedStatus(0);
    }
    else
    {
      digitalWrite(LED_BUILTIN2, HIGH);
      funs->writeLedStatus(1);
    }

    break;
  case cmdCamUpdateMode:
    funs->writeUpdateMode(1);
    ESP.restart();
    break;
  default:
    break;
  }
}
