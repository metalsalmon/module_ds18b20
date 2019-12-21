#ifndef FW_UPDATER_HPP
#define FW_UPDATER_HPP

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

class FW_updater{
  public:
    FW_updater(const char gw_ip[], const unsigned short port = 80);
    bool update();
  
  private:
    WiFiClient wifi_client;
    char* url;
};

#endif