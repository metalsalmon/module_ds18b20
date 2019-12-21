#include <Arduino.h>
#include <WiFi.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include "MQTT_resolver.cpp"

const char* ssid     = "Wifina";
const char* password = "martinko";

const char* module_id   = "DUMMY_ID";
const char* module_type = "DUMMY_TYPE";

MQTT_client *mqtt_client;
MQTT_resolver* mqtt_resolver;
FW_updater *fw_updater;

void setup(){
  Serial.begin(115200);
  delay(10);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);  
  }

  char* gateway_ip;
  IPAddress gw = WiFi.gatewayIP();
  asprintf(&gateway_ip, "%d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);

  fw_updater = new FW_updater(gateway_ip, 5000);
  
  mqtt_client = new MQTT_client("192.168.1.6");
  mqtt_resolver = new MQTT_resolver(*fw_updater, *mqtt_client);

  mqtt_client->set_mqtt_params(module_id, mqtt_resolver->resolve);
  mqtt_client->connect();
  
  char* connect_msg;
  asprintf(&connect_msg, "{ \"uuid\": \"%s\", \"type\": \"%s\" }", module_id, module_type);
  mqtt_client->publish("MODULE_ID", connect_msg);
  free(connect_msg);
  
  mqtt_client->subscribe("ALL_MODULES", 2);
  mqtt_client->subscribe(String(module_id) + "/SET_CONFIG", 2);
  mqtt_client->subscribe(String(module_id) + "/SET_VALUE", 2);
}

void loop(){
    mqtt_client->mqtt_loop();
}
