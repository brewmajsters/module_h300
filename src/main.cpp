#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include <MD5.hpp>
#include <H300.hpp>

// #define DEBUG Serial.println(message);

////////////////////////////////////////////////////////////////////////////////
/// CONSTANT DEFINITION
////////////////////////////////////////////////////////////////////////////////

static constexpr char* const wifi_ssid     = "SSID";
static constexpr char* const wifi_password = "PASSWORD";

static constexpr char* const module_id   = "DUMMY_ID";
static constexpr char* const module_type = "VFD_H300";

static constexpr uint32_t loop_delay_ms = 10;

static constexpr uint16_t fw_update_port = 5000;

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL OBJECTS
////////////////////////////////////////////////////////////////////////////////

static FW_updater  *fw_updater = nullptr;
static MQTT_client *mqtt_client = nullptr;

static std::vector<H300> devices;

static void resolve_mqtt(String& topic, String& payload);

////////////////////////////////////////////////////////////////////////////////
/// SETUP
////////////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  delay(10);

  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  const String gateway_ip = WiFi.gatewayIP().toString();

  fw_updater = new FW_updater(gateway_ip.c_str(), fw_update_port);

  mqtt_client = new MQTT_client("192.168.1.65");
  mqtt_client->set_mqtt_params(module_id, module_type, resolve_mqtt);
  mqtt_client->connect();
  mqtt_client->publish_module_id();
  mqtt_client->subscribe("ALL_MODULES");
  mqtt_client->subscribe((std::string(module_id) + "/SET_CONFIG").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/SET_VALUE").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/UPDATE_FW").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/REQUEST").c_str());  
}

////////////////////////////////////////////////////////////////////////////////
/// LOOP
////////////////////////////////////////////////////////////////////////////////

void loop() {

  mqtt_client->mqtt_loop();

  if (!devices.empty()) {
    DynamicJsonDocument json(512);

    for (H300& device : devices) {
      if (device.decrease_counter()) {
        JsonObject device_object = json.createNestedObject(device.device_uuid);

        uint16_t speed = 0;
        if (device.read_value(H300::speed_register, &speed)) {
          device_object["SPEED"] = speed;
        }
        
        uint16_t get_motion = 0;
        if (device.read_value(H300::get_motion_register, &get_motion)) {
          device_object["GET_MOTION"] = get_motion;
        }

        uint16_t state = 0;
        if (device.read_value(H300::state_register, &state)) {
          device_object["STATE"] = state;
        }

        uint16_t main_freq = 0;
        if (device.read_value(H300::main_freq_register, &main_freq)) {
          device_object["MAIN_FREQUENCY"] = main_freq;
        }

        uint16_t aux_freq = 0;
        if (device.read_value(H300::aux_freq_register, &aux_freq)) {
          device_object["AUX_FREQUENCY"] = aux_freq;
        }
      }    
    }

    if (!json.isNull()) {
      mqtt_client->publish_value_update(json); 
    }
  }

  delay(loop_delay_ms);
}

////////////////////////////////////////////////////////////////////////////////
/// MQTT RESOLVER
////////////////////////////////////////////////////////////////////////////////

static void resolve_mqtt(String& topic, String& payload) {

  Serial.println("incoming: " + topic + " - " + payload);

  DynamicJsonDocument payload_json(256);
  DeserializationError json_err = deserializeJson(payload_json, payload);

  if (json_err) {
    Serial.println("JSON error: " + String(json_err.c_str()));
    return;
  }

  if (topic.equals("ALL_MODULES") || topic.equals(String(module_id) + "/REQUEST")) {
    const char* request = payload_json["request"];

    if (request != nullptr) {
      if (String(request) == "module_discovery") {
        mqtt_client->publish_module_id();
      } else if (String(request) == "shutdown") {
        // TODO: CUSTOM SHUTDOWN ACTION
      } else if (String(request) == "init") {
        // TODO: CUSTOM INIT ACTION
      }
    }
  } else if (topic.equals(String(module_id) + "/SET_CONFIG")) {
    JsonObject json_config = payload_json.as<JsonObject>();
    std::vector<H300>().swap(devices);    

    for (const JsonPair pair : json_config) {

      const char* const device_uuid = pair.key().c_str();
      const JsonObject device_config = pair.value().as<JsonObject>();
      const uint8_t unit_id = device_config["address"];
      const uint16_t poll_rate = device_config["poll_rate"];

      devices.emplace_back(device_uuid, unit_id, (poll_rate * 1000) / loop_delay_ms);
    }

    char* const payload_cpy = strdup(payload.c_str());
    unsigned char* const md5_hash = MD5::make_hash(payload_cpy);
    char* const md5_str = MD5::make_digest(md5_hash, 16);
    mqtt_client->publish_config_update(md5_str);

    free(payload_cpy);
    free(md5_hash);
    free(md5_str);
  } else if (topic.equals(String(module_id) + "/SET_VALUE")) {

    const char* device_uuid = payload_json["device_uuid"];
    const char* datapoint = payload_json["datapoint"];
    const uint16_t value = payload_json["value"];    

    Serial.print("device_id: " + String(device_uuid));
    Serial.print(" datapoint: " + String(datapoint));
    Serial.println(" value: " + String(value));

    for (H300 device : devices) {
      if (device.device_uuid == device_uuid) {
        if (String(datapoint).equals("SPEED")) {
          bool res = device.write_value(H300::speed_register, value);
          if (!res) 
            Serial.println("Error setting speed");
        } else if (String(datapoint).equals("SET_MOTION")) {
          bool res = device.write_value(H300::set_motion_register, value);
          if (!res) 
            Serial.println("Error setting motion");  
        } else {
          Serial.println("Error: unrecognized datapoint");
        }

        break;   
      }   
    }
  } else if (topic.equals(String(module_id) + "/UPDATE_FW")) {
    const char* version = payload_json["version"];
    fw_updater->update(version);
  }
}
