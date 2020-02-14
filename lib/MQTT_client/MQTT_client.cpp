#include "MQTT_client.hpp"

MQTT_client::MQTT_client(const char gw_ip[], const uint32_t port, const uint16_t buffer_size){
  mqtt_client = new MQTTClient(buffer_size);
  mqtt_client->begin(gw_ip, port, wifi_client);      
  mqtt_client->setOptions(5, true, 1000);
}

void MQTT_client::set_mqtt_params(const char module_id[], const char module_type[], MQTTClientCallbackSimple callback){
  this->module_id = strdup(module_id);
  this->module_type = strdup(module_type);
  
  char lw_msg[256];
  StaticJsonDocument<JSON_OBJECT_SIZE(1) + 150> json;
  json["module_uuid"] = module_id;
  serializeJson(json, lw_msg);

  mqtt_client->onMessage(callback);
  mqtt_client->setWill("MODULE_DISCONNECT", lw_msg, false, 2);
}

void MQTT_client::connect(){
  if (!strlen(module_id)){
    throw std::runtime_error("Module ID not specified");
  }
  
  while (!mqtt_client->connect(module_id, false)) {
    delay(1000);
  }
}

bool MQTT_client::subscribe(const char topic[], const uint8_t QOS){
  return mqtt_client->subscribe(topic, QOS);
}

bool MQTT_client::publish(const char topic[], const char payload[], const uint8_t QOS){
  return mqtt_client->publish(topic, payload, false, QOS);
}

bool MQTT_client::publish_module_id(const uint8_t QOS){
  char msg[256];
  StaticJsonDocument<JSON_OBJECT_SIZE(2) + 256> json;
  json["module_uuid"] = module_id;
  json["module_type"] = module_type;
  serializeJson(json, msg);

  bool res = publish("MODULE_ID", msg, QOS);
  
  return res;
}

bool MQTT_client::publish_config_update(const char config_hash[], const uint8_t QOS){
  char msg[256];
  StaticJsonDocument<JSON_OBJECT_SIZE(2) + 256> json;
  json["module_uuid"] = module_id;
  json["config_hash"] = config_hash;
  serializeJson(json, msg);

  bool res = publish("MODULE_CONFIG_UPDATE", msg, QOS);
  
  return res;
}

bool MQTT_client::publish_value_update(DynamicJsonDocument values_json, const uint8_t QOS){
  char msg[256];
  DynamicJsonDocument json(512);
  json["module_uuid"] = module_id;
  json["values"] = values_json;
  serializeJson(json, msg);

  bool res = publish("VALUE_UPDATE", msg, QOS);
  
  return res;
}

bool MQTT_client::mqtt_loop(){
  return mqtt_client->loop();  
}    

MQTT_client::~MQTT_client(){
  mqtt_client->disconnect(); 
  free(mqtt_client);
  free(module_id);
  free(module_type);
}