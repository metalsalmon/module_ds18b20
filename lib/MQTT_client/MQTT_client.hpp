#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP

#include <WiFi.h>
#include <MQTT.h>

class MQTT_client{
  public:
    MQTT_client(const char gw_ip[], const unsigned int port = 1883);

    void set_mqtt_params(const char module_id[], MQTTClientCallbackSimple callback);
    void connect();
    void subscribe(const String &topic, const unsigned int QOS = 2);
    void publish(const String &topic, const String &payload, const unsigned int QOS = 2);
    void mqtt_loop();

    ~MQTT_client();

  private:
    char* module_id = nullptr;
    WiFiClient wifi_client;
    MQTTClient mqtt_client;
};

#endif