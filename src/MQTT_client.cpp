#include <WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>

class MQTT_client{
  public:
    MQTT_client(WiFiClient *wifi_client, const char gw_ip[], const unsigned int port = 1883)
    : wifi_client(wifi_client)
    {
      mqtt_client.begin(gw_ip, port, *wifi_client);      
      mqtt_client.setOptions(5, true, 1000);
    }

    void set_mqtt_params(const char module_id[], const char module_type[], MQTTClientCallbackSimple callback){
      asprintf(&this->module_id, module_id);
      asprintf(&this->module_type, module_type);

      mqtt_client.onMessage(callback);
      mqtt_client.setWill("MODULE_DISCONNECTED", "{ \"status\": \"disconnected\" }", false, 2);
    }

    void connect(){
      if (module_id == nullptr || module_type == nullptr){
        throw std::runtime_error("MQTT params not specified");
      }
      
      while (!mqtt_client.connect(module_id, false)) {
        delay(1000);
      }

      char* msg;
      asprintf(&msg, "{ \"uuid\": \"%s\", \"type\": \"%s\" }", module_id, module_type);
      publish("MODULE_ID", msg);
    }

    void subscribe(const String &topic, const unsigned int QOS = 2){
      mqtt_client.subscribe(topic, QOS);
    }

    void publish(const String &topic, const String &payload, const unsigned int QOS = 2){
      mqtt_client.publish(topic, payload, false, QOS);
    }

    void mqtt_loop(){
      mqtt_client.loop();  
    }    

    ~MQTT_client(){
      mqtt_client.disconnect(); 
    }

  private:
    char* module_id = nullptr;
    char* module_type = nullptr;
    WiFiClient *wifi_client;
    MQTTClient mqtt_client;
};
