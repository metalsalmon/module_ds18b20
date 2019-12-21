#include <HTTPClient.h>
#include <HTTPUpdate.h>

class FW_update{
  public:
    FW_update(const char gw_ip[], WiFiClient *wifi_client, const unsigned short port = 80) 
    : wifi_client(wifi_client)
    {
      httpUpdate.setLedPin(2, LOW);

      asprintf(&url, "http://%s:%u/module_firmware", gw_ip, port);
    }

    bool update(){
      t_httpUpdate_return ret = httpUpdate.update(*wifi_client, url);

      return ret != HTTP_UPDATE_OK ? true : false;
    }

  private:
    WiFiClient *wifi_client;
    char* url;
};
