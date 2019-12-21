#include "MQTT_client.hpp"

MQTT_client::MQTT_client(const char gw_ip[], const unsigned int port){
  mqtt_client.begin(gw_ip, port, wifi_client);      
  mqtt_client.setOptions(5, true, 1000);
}

void MQTT_client::set_mqtt_params(const char module_id[], MQTTClientCallbackSimple callback){
  asprintf(&this->module_id, module_id);

  char* lw_msg;
  asprintf(&lw_msg, "{ \"uuid\": \"%s\" }", module_id);

  mqtt_client.onMessage(callback);
  mqtt_client.setWill("MODULE_DISCONNECTED", std::string(lw_msg).c_str(), false, 2);

  free(lw_msg);
}

void MQTT_client::connect(){
  if (module_id == nullptr ){
    throw std::runtime_error("Module ID not specified");
  }
  
  while (!mqtt_client.connect(module_id, false)) {
    delay(1000);
  }
}

void MQTT_client::subscribe(const String &topic, const unsigned int QOS){
  mqtt_client.subscribe(topic, QOS);
}

void MQTT_client::publish(const String &topic, const String &payload, const unsigned int QOS){
  mqtt_client.publish(topic, payload, false, QOS);
}

void MQTT_client::mqtt_loop(){
  mqtt_client.loop();  
}    

MQTT_client::~MQTT_client(){
  mqtt_client.disconnect(); 
}