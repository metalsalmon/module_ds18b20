#include <Arduino.h>
#include <WiFi.h>
#include "FW_update.cpp"
#include "MQTT_client.cpp"

const char* ssid     = "Wifina";
const char* password = "martinko";

const char* module_id = "DUMMY_ID";
const char* module_type = "DUMMY_TYPE";

WiFiClient wifi_client;
MQTT_client *mqtt_client;
FW_update *fw_updater;

char* gateway_ip;

void mqtt_callback(String &topic, String &payload);

void setup(){
  Serial.begin(115200);
  delay(10);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);  
  }

  Serial.println(WiFi.localIP());

  IPAddress gw = WiFi.gatewayIP();

  asprintf(&gateway_ip, "%d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);

  fw_updater = new FW_update(gateway_ip, &wifi_client, 5000);
  
  mqtt_client = new MQTT_client(&wifi_client, "192.168.1.6");
  mqtt_client->set_mqtt_params(module_id, module_type, mqtt_callback);
  mqtt_client->connect();
  mqtt_client->subscribe("ALL_MODULES", 2);

//    bool updated = fw_updater->update();
}

void loop(){
    mqtt_client->mqtt_loop();
}

void mqtt_callback(String &topic, String &payload){
  Serial.println("incoming: " + topic + " - " + payload);
}
