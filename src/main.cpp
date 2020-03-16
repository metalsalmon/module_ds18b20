#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include <MD5.hpp>

// debug mode, set to 0 if making a release
#define DEBUG 1

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

#define MODULE_TYPE  "DUMMY_TYPE"

#define LOOP_DELAY_MS   10u
#define FW_UPDATE_PORT  5000u

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL OBJECTS
////////////////////////////////////////////////////////////////////////////////

static String module_mac;

static FW_updater  *fw_updater = nullptr;
static MQTT_client *mqtt_client = nullptr;

static bool standby_mode = false;

static void resolve_mqtt(String& topic, String& payload);

////////////////////////////////////////////////////////////////////////////////
/// SETUP
////////////////////////////////////////////////////////////////////////////////

void setup() {

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
  LOG("Subscribing to " + module_mac + "/SET_VALUE ...");
  mqtt_client->subscribe((module_mac + "/SET_VALUE").c_str(), 2u);
  LOG("Subscribing to " + module_mac + "/UPDATE_FW ...");
  mqtt_client->subscribe((module_mac + "/UPDATE_FW").c_str(), 2u);
  LOG("Subscribing to " + module_mac + "/REQUEST ...");
  mqtt_client->subscribe((module_mac + "/REQUEST").c_str(), 2u);
}

////////////////////////////////////////////////////////////////////////////////
/// LOOP
////////////////////////////////////////////////////////////////////////////////

void loop() {

  mqtt_client->loop();

  // DynamicJsonDocument json(512);
  // JsonObject device_1_obj = json.createNestedObject("[DEVICE_ID]");
  // device_1_obj["datapoint_code_1"] = 22.3;
  // device_1_obj["datapoint_code_2"] = "string";
  // device_1_obj["datapoint_code_3"] = true;
  // JsonObject device_2_obj = json.createNestedObject("[DEVICE_ID]");
  // device_2_obj["datapoint_code_1"] = 22.3;
  // device_2_obj["datapoint_code_2"] = "string";
  // device_2_obj["datapoint_code_3"] = true;
  // mqtt_client->publish_value_update(json);

  delay(LOOP_DELAY_MS);
}

////////////////////////////////////////////////////////////////////////////////
/// MQTT RESOLVER
////////////////////////////////////////////////////////////////////////////////

static void resolve_mqtt(String& topic, String& payload) {

  LOG("Received message: " + topic + " - " + payload);

  DynamicJsonDocument payload_json(256);
  DeserializationError json_err = deserializeJson(payload_json, payload);

  if (json_err) {
    LOG("JSON error: " + String(json_err.c_str()));
    return;
  }

  if (topic.equals("ALL_MODULES") || topic.equals(module_mac + "/REQUEST")) {
    const char* request = payload_json["request"];

    if (request != nullptr) {
      if (String(request) == "module_discovery") {
        mqtt_client->publish_module_id();
      } else if (String(request) == "stop") {
        LOG("Switching to standby mode");
        // TODO: CUSTOM STOP ACTION
        standby_mode = true;
      } else if (String(request) == "start") {
        LOG("Switching to active mode");
        // switch to active mode
        standby_mode = false;
      }
    }
  } else if (topic.equals(module_mac + "/SET_CONFIG")) {    
    JsonObject json_config = payload_json.as<JsonObject>();
    LOG("Deleting previous configuration");
    // TODO: DELETE PREVIOUS CONFIGURATION

    // create devices according to received configuration
    for (const JsonPair& pair : json_config) { 

      const char* const device_uuid = pair.key().c_str();
      const JsonObject device_config = pair.value().as<JsonObject>();
      // TODO: CUSTOM DEVICE ADDRESS ASSIGNMENT (ACCORDING TO DATATYPE AND RANGE)
      // E.G: const uint8_t address = device_config["address"];

      const uint16_t poll_rate = device_config["poll_rate"];

      LOG("Creating device with parameters: ");
      LOG(String("\t uuid:\t") + device_uuid);
      // LOG(String("\t address:\t") + address);
      LOG(String("\t interval_rate:\t") + ((poll_rate * 1000) / LOOP_DELAY_MS));

      // TODO: CUSTOM DEVICE CREATION ACTION
    }

    // calculate config MD5 chuecksum
    std::string payload_cpy(payload.c_str());
    const std::string& md5_str = MD5::make_digest(MD5::make_hash(&payload_cpy[0]), 16);

    LOG(String("Config MD5 checksum: ") + md5_str.c_str());

    mqtt_client->publish_config_update(md5_str);
  } else if (topic.equals(module_mac + "/SET_VALUE")) {

    const char* device_uuid = payload_json["device_uuid"];
    const char* datapoint = payload_json["datapoint"];
    // TODO: CUSTOM VALUE ASSIGNMENT (ACCORDING TO DATATYPE AND RANGE)
    // E.G: const uint8_t value = payload_json["value"];   

    LOG("Setting value:");
    LOG(String("\t device_uuid: ") + device_uuid);
    LOG(String("\t datapoint: ") + datapoint);
    // LOG(String("\t value: ") + value);

    // TODO: CUSTOM SET_VALUE ACTION
  } else if (topic.equals(module_mac + "/UPDATE_FW")) {
    const char* version = payload_json["version"];

    LOG(String("Updating firmware to version: ") + version);
    bool res = fw_updater->update(version);
    if (!res) {
      LOG("\t result: error");
    } else {
      LOG("\t result: ok");
    }
  }
}
