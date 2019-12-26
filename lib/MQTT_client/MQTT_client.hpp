#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP

#include <WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>

class MQTT_client{
  public:
    MQTT_client(const char gw_ip[], const uint32_t port = 1883, const uint16_t buffer_size = 256);

    void set_mqtt_params(const char module_id[], const char module_type[], MQTTClientCallbackSimple callback);
    void connect();
    bool subscribe(const char topic[], const uint8_t QOS = 2);
    bool publish(const char topic[], const char payload[], const uint8_t QOS = 2);
    bool publish_module_id(const uint8_t QOS = 2);
    bool publish_config_update(const char config_hash[], const uint8_t QOS = 2);
    bool publish_value_update(const uint32_t device_id, DynamicJsonDocument datapoints_json, const uint8_t QOS = 0);
    bool mqtt_loop();

    ~MQTT_client();

  private:
    char* module_id = nullptr;
    char* module_type = nullptr;
    WiFiClient wifi_client;
    MQTTClient* mqtt_client = nullptr;
};

#endif