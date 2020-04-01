#pragma once

#define ARDUINOJSON_ENABLE_STD_STRING 1

#include <WiFi.h>
#include <MQTT.h>
#include <string>
#include <ArduinoJson.h>

class MQTT_client : public MQTTClient 
{
  public:
    MQTT_client(const char* gw_ip, const uint32_t port = 1883, const uint16_t buffer_size = 256);

    void setup_mqtt(
      const std::string& module_mac, 
      const std::string& module_type, 
      MQTTClientCallbackSimple callback
    );
    bool publish_module_id(const uint8_t QOS = 2);
    bool publish_config_update(const std::string& config_hash, const uint8_t QOS = 2);
    bool publish_value_update(DynamicJsonDocument values_json, const uint8_t QOS = 0);
    bool publish_request_result(
      const uint16_t sequence_number, 
      const bool result, 
      const std::string& details = "",
      const uint8_t QOS = 1
    );

    ~MQTT_client();

  private:
    std::string module_mac;
    std::string module_type;
    WiFiClient wifi_client;
};