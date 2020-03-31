#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include <MD5.hpp>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS18B20.cpp>

// debug mode, set to 0 if making a release
#define DEBUG 0

// Logging macro used in debug mode
#if DEBUG == 1
  #define LOG(message) Serial.println(message);
#else
  #define LOG(message)
#endif

////////////////////////////////////////////////////////////////////////////////
/// CONSTANT DEFINITION
////////////////////////////////////////////////////////////////////////////////

#define WIFI_SSID    "SSID"
#define WIFI_PASS    "PASS"

#define MODULE_TYPE  "DS18B20"

#define ONE_WIRE_PIN 4
#define LOOP_DELAY_MS   10u
#define FW_UPDATE_PORT  5000u

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL OBJECTS
////////////////////////////////////////////////////////////////////////////////

static String module_mac;

static FW_updater  *fw_updater = nullptr;
static MQTT_client *mqtt_client = nullptr;

static OneWire one_wire(ONE_WIRE_PIN);
static DallasTemperature devices_bus(&one_wire);
static std::vector<DS18B20> devices;

static bool standby_mode = false;

static void resolve_mqtt(String& topic, String& payload);

////////////////////////////////////////////////////////////////////////////////
/// SETUP
////////////////////////////////////////////////////////////////////////////////

void setup() 
{

  #if DEBUG == true
    Serial.begin(115200);
  #endif
  delay(10);

  module_mac = WiFi.macAddress();
  LOG("Module MAC: " + module_mac);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);
  LOG("Connected to Wi-Fi AP");

  const String gateway_ip = WiFi.gatewayIP().toString();
  LOG("GW IP address: " + gateway_ip);

  // firmware server expected to run on GW
  fw_updater = new FW_updater(gateway_ip.c_str(), FW_UPDATE_PORT);

  // MQTT broker expected to run on GW
  mqtt_client = new MQTT_client(gateway_ip.c_str());
  mqtt_client->setup_mqtt(module_mac.c_str(), MODULE_TYPE, resolve_mqtt);
  LOG("Connected to MQTT broker");
  mqtt_client->publish_module_id();
  LOG("Subscribing to ALL_MODULES ...");
  mqtt_client->subscribe("ALL_MODULES");
  LOG("Subscribing to " + module_mac + "/SET_CONFIG ...");
  mqtt_client->subscribe((module_mac + "/SET_CONFIG").c_str(), 2u);
  LOG("Subscribing to " + module_mac + "/UPDATE_FW ...");
  mqtt_client->subscribe((module_mac + "/UPDATE_FW").c_str(), 2u);
  LOG("Subscribing to " + module_mac + "/REQUEST ...");
  mqtt_client->subscribe((module_mac + "/REQUEST").c_str(), 2u);
}

////////////////////////////////////////////////////////////////////////////////
/// LOOP
////////////////////////////////////////////////////////////////////////////////

void loop() 
{

  mqtt_client->loop();

  if (!devices.empty() && !standby_mode)
  {
    DynamicJsonDocument json(512);

    for(DS18B20& device : devices)
    {
      if(!device.decrease_counter()) continue;
      devices_bus.requestTemperaturesByAddress(device.address);
      JsonObject device_obj = json.createNestedObject(device.device_uuid.c_str());
      device_obj["TEMP"] = devices_bus.getTempC(device.address);
    }

    if(!json.isNull()){
      #if DEBUG == 1
        String json_log;
        serializeJson(json, json_log);
        LOG(json_log);
      #endif
      mqtt_client->publish_value_update(json);
    }
  }
  delay(LOOP_DELAY_MS);
}

////////////////////////////////////////////////////////////////////////////////
/// MQTT RESOLVER
////////////////////////////////////////////////////////////////////////////////

static void resolve_mqtt(String& topic, String& payload) 
{

  LOG("Received message: " + topic + " - " + payload);

  DynamicJsonDocument payload_json(256);
  DeserializationError json_err = deserializeJson(payload_json, payload);

  if (json_err) 
  {
    LOG("JSON error: " + String(json_err.c_str()));
    return;
  }

  if (topic.equals("ALL_MODULES") || topic.equals(module_mac + "/REQUEST")) 
  {
    const char* request = payload_json["request"];

    if (request != nullptr) 
    {
      if (String(request) == "module_discovery") 
        mqtt_client->publish_module_id();
      else if (String(request) == "stop")
      {
        const uint16_t sequence_number = payload_json["sequence_number"];
        LOG("Switching to standby mode");
        standby_mode = true;
        mqtt_client->publish_request_result(sequence_number, true, "", 1);
      } 
      else if (String(request) == "start") 
      {
        const uint16_t sequence_number = payload_json["sequence_number"];
        LOG("Switching to active mode");
        standby_mode = false;
        mqtt_client->publish_request_result(sequence_number, true, "", 1);
      }
    }
  } 
  else if (topic.equals(module_mac + "/SET_CONFIG")) 
  {    
    JsonObject json_config = payload_json.as<JsonObject>();
    LOG("Deleting previous configuration");
    std::vector<DS18B20>().swap(devices);

    for (const JsonPair& pair : json_config) 
    { 
      const char* device_uuid = pair.key().c_str();
      const JsonObject device_config = pair.value().as<JsonObject>();
      const uint16_t poll_rate = device_config["poll_rate"];
      const char* address = device_config["address"];
      LOG("Creating device with parameters: ");
      LOG(String("\t uuid:\t") + device_uuid);
      //LOG(String("\t address:\t") + address);
      LOG(String("\t interval_rate:\t") + (poll_rate * 1000) / LOOP_DELAY_MS);
      devices.emplace_back(device_uuid, poll_rate * 1000/LOOP_DELAY_MS, address);
    }

    // calculate config MD5 chuecksum
    std::string payload_cpy(payload.c_str());
    const std::string& md5_str = MD5::make_digest(MD5::make_hash(&payload_cpy[0]), 16);

    LOG(String("Config MD5 checksum: ") + md5_str.c_str());

    mqtt_client->publish_config_update(md5_str);
  } 
  else if (topic.equals(module_mac + "/UPDATE_FW")) 
  {
    const char* version = payload_json["version"];
    const uint16_t sequence_number = payload_json["sequence_number"];

    LOG(String("Updating firmware to version: ") + version);
    bool result = fw_updater->update(version);
    String log_msg = result ? "\t result: ok" : "\t result: error";
    LOG(log_msg);
    
    mqtt_client->publish_request_result(sequence_number, result);
  }
}
