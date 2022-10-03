#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include "variables.h"
#include "functions.h"
#include <WiFi.h>
#define LED_BUILTIN2 4
#define CHANNEL 0

class nowLib;
typedef struct broadcast_message
{
    char device[16]="unkown";
    char type[16] = "unknown"; // data or broadcast
    char ssid[16] = "slave";   // slave or master
    char pass[16] = "unknown"; 
    uint8_t datalen = 1;
    char data[128] = " ";
} broadcast_message;
    static variables *svars;
    static nowLib *snow;
    static functions *sfuns;
class nowLib
{

private:
public:
broadcast_message myData;
    nowLib();
       variables *vars;
     nowLib *now;
     functions *funs;

    void init(variables *VARS,functions *FUNS,nowLib *NOW);
    esp_now_peer_info_t slave;
    uint8_t initialized=0;
    
    void addBroadcastPeer();
   static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
   static void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
   void send(std::string incomingdata, uint8_t len);
   void broadcast();
   void processData(std::string value);
};