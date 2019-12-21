#include <ArduinoJson.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>

class MQTT_resolver{
  public:
    MQTT_resolver(FW_updater& fw_updater, MQTT_client& mqtt_client)
    : fw_updater(fw_updater), mqtt_client(mqtt_client)
    {}
    
    static void resolve(String& topic, String& payload){
      Serial.println("incoming: " + topic + " - " + payload); 

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      const char* request = doc["request"];

      Serial.println(request);
    }
  
  private:
    const FW_updater& fw_updater;
    const MQTT_client& mqtt_client;
};