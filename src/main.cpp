#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include <MD5.hpp>

const char* ssid     = "Wifina";
const char* password = "martinko";

const char* module_id   = "DUMMY_ID";
const char* module_type = "DUMMY_TYPE";

FW_updater  *fw_updater = nullptr;
MQTT_client *mqtt_client = nullptr;

void resolve_mqtt(String& topic, String& payload);

void setup(){
  Serial.begin(115200);
  delay(10);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);  
  }

  char* gateway_ip = nullptr;
  IPAddress gw = WiFi.gatewayIP();
  asprintf(&gateway_ip, "%d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);

  fw_updater = new FW_updater(gateway_ip, 5000);
  
  mqtt_client = new MQTT_client("192.168.1.6");
  // mqtt_client = new MQTT_client(gateway_ip);
  mqtt_client->set_mqtt_params(module_id, module_type, resolve_mqtt);
  mqtt_client->connect();
  mqtt_client->publish_module_id();
  mqtt_client->subscribe("ALL_MODULES");
  mqtt_client->subscribe((std::string(module_id) + "/SET_CONFIG").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/SET_VALUE").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/UPDATE_FW").c_str());
}

void loop(){
  mqtt_client->mqtt_loop();

  // DynamicJsonDocument json(512);
  // json["datapoint_code_1"] = 22.3;
  // json["datapoint_code_2"] = "string";
  // json["datapoint_code_3"] = true;
  // mqtt_client->publish_value_update(12, json);
  
  // delay(1000);
}

void resolve_mqtt(String& topic, String& payload){
  Serial.println("incoming: " + topic + " - " + payload); 

  DynamicJsonDocument payload_json(256);
  deserializeJson(payload_json, payload);

  if (topic.equals("ALL_MODULES")){
    const char* request = payload_json["request"];

    if (request != nullptr){
      if (String(request) == "module_discovery"){
        mqtt_client->publish_module_id();
      } else if (String(request) == "shutdown"){
        // TODO: CUSTOM SHUTDOWN ACTION
      } 
    }
  } else if (topic.equals(String(module_id) + "/SET_CONFIG")){    
    // TODO: CUSTOM SET_CONFIG ACTION

    char* payload_cpy = strdup(payload.c_str());
    unsigned char* md5_hash = MD5::make_hash(payload_cpy);
    char* md5_str = MD5::make_digest(md5_hash, 16);
    mqtt_client->publish_config_update(md5_str);
    
    free(payload_cpy);
    free(md5_hash);
    free(md5_str);
  } else if (topic.equals(String(module_id) + "/SET_VALUE")){
    const uint32_t device_id = payload_json["device_id"];
    const char* datapoint = payload_json["datapoint"];

    // TODO: CUSTOM SET_VALUE ACTION
  } else if (topic.equals(String(module_id) + "/UPDATE_FW")){
    const char* version = payload_json["version"];

    fw_updater->update(version);
  }
}