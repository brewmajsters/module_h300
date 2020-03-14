#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include <MD5.hpp>
#include <H300.hpp>

// debug mode, set to false if making a release
#define DEBUG true

// Logging macro used in debug mode
#if DEBUG == true
  #define LOG(message) Serial.println(message);
#else
  #define LOG(message)
#endif

////////////////////////////////////////////////////////////////////////////////
/// CONSTANT DEFINITION
////////////////////////////////////////////////////////////////////////////////

#define WIFI_SSID    "SSID"
#define WIFI_PASS    "PASSWORD"

#define MODULE_UUID  "DUMMY_UUID"
#define MODULE_TYPE  "VFD_H300"

#define LOOP_DELAY_MS   10
#define FW_UPDATE_PORT  5000

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL OBJECTS
////////////////////////////////////////////////////////////////////////////////

static FW_updater  *fw_updater = nullptr;
static MQTT_client *mqtt_client = nullptr;

static std::vector<H300> devices;

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

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);
  LOG("Connected to Wi-Fi AP");

  const String gateway_ip = WiFi.gatewayIP().toString();
  LOG("GW IP address: " + gateway_ip);

  // firmware server expected to run on GW
  fw_updater = new FW_updater(gateway_ip.c_str(), FW_UPDATE_PORT);

  // MQTT broker expected to run on GW
  mqtt_client = new MQTT_client("192.168.1.65");
  mqtt_client->set_mqtt_params(MODULE_UUID, MODULE_TYPE, resolve_mqtt);
  mqtt_client->connect();
  LOG("Connected to MQTT broker");
  mqtt_client->publish_module_id();
  LOG("Subscribing to ALL_MODULES ...");
  mqtt_client->subscribe("ALL_MODULES");
  LOG(String("Subscribing to ") + MODULE_UUID + "/SET_CONFIG ...");
  mqtt_client->subscribe((std::string(MODULE_UUID) + "/SET_CONFIG").c_str());
  LOG(String("Subscribing to ") + MODULE_UUID + "/SET_VALUE ...");
  mqtt_client->subscribe((std::string(MODULE_UUID) + "/SET_VALUE").c_str());
  LOG(String("Subscribing to ") + MODULE_UUID + "/UPDATE_FW ...");
  mqtt_client->subscribe((std::string(MODULE_UUID) + "/UPDATE_FW").c_str());
  LOG(String("Subscribing to ") + MODULE_UUID + "/REQUEST ...");
  mqtt_client->subscribe((std::string(MODULE_UUID) + "/REQUEST").c_str());  
}

////////////////////////////////////////////////////////////////////////////////
/// LOOP
////////////////////////////////////////////////////////////////////////////////

void loop() {

  mqtt_client->mqtt_loop();

  // check if any device is present in config and standby mode is off
  if (!devices.empty() && !standby_mode) {
    // prepare json payload
    DynamicJsonDocument json(512);

    // loop through device vector
    for (H300& device : devices) {
      // check if device is expected to be read (dependent on its poll rate)
      if (device.decrease_counter()) {
        LOG("Reading device: " + String(device.device_uuid.c_str()));
        JsonObject device_object = json.createNestedObject(device.device_uuid);

        uint16_t speed = 0;
        if (device.read_value(H300::speed_register, &speed)) {
          device_object["SPEED"] = speed;
        }
        LOG("\t SPEED: " + speed);
        
        uint16_t get_motion = 0;
        if (device.read_value(H300::get_motion_register, &get_motion)) {
          device_object["GET_MOTION"] = get_motion;
        }
        LOG("\t GET_MOTION: " + get_motion);

        uint16_t state = 0;
        if (device.read_value(H300::state_register, &state)) {
          device_object["STATE"] = state;
        }
        LOG("\t STATE: " + state);

        uint16_t main_freq = 0;
        if (device.read_value(H300::main_freq_register, &main_freq)) {
          device_object["MAIN_FREQUENCY"] = main_freq;
        }
        LOG("\t MAIN_FREQUENCY: " + main_freq);

        uint16_t aux_freq = 0;
        if (device.read_value(H300::aux_freq_register, &aux_freq)) {
          device_object["AUX_FREQUENCY"] = aux_freq;
        }
        LOG("\t AUX_FREQUENCY: " + aux_freq);
      }    
    }

    // publish only if at least one device was read
    if (!json.isNull()) {
      mqtt_client->publish_value_update(json); 
    }
  }

  delay(LOOP_DELAY_MS);
}

////////////////////////////////////////////////////////////////////////////////
/// MQTT RESOLVER
////////////////////////////////////////////////////////////////////////////////

static void resolve_mqtt(String& topic, String& payload) {

  LOG("incoming: " + topic + " - " + payload);

  DynamicJsonDocument payload_json(256);
  DeserializationError json_err = deserializeJson(payload_json, payload);

  if (json_err) {
    LOG("JSON error: " + String(json_err.c_str()));
    return;
  }

  if (topic.equals("ALL_MODULES") || topic.equals(String(MODULE_UUID) + "/REQUEST")) {
    const char* request = payload_json["request"];

    if (request != nullptr) {
      if (String(request) == "module_discovery") {
        mqtt_client->publish_module_id();
      } else if (String(request) == "shutdown") {
        // stop all motors using DC breaks and switch to standy mode
        for (H300& device : devices) {
          device.write_value(H300::set_motion_register, 6);
        }
        standby_mode = true;
      } else if (String(request) == "init") {
        // switch off standby mode
        standby_mode = false;
      }
    }
  } else if (topic.equals(String(MODULE_UUID) + "/SET_CONFIG")) {
    JsonObject json_config = payload_json.as<JsonObject>();
    std::vector<H300>().swap(devices); // delete previous configuration

    // create devices according to received configuration
    for (const JsonPair pair : json_config) { 

      const char* const device_uuid = pair.key().c_str();
      const JsonObject device_config = pair.value().as<JsonObject>();
      const uint8_t unit_id = device_config["address"];
      const uint16_t poll_rate = device_config["poll_rate"];

      LOG("Creating device with parameters: ");
      LOG(String("\t uuid:") + device_uuid);
      LOG(String("\t unit_id:") + unit_id);
      LOG(String("\t interval_rate:") + ((poll_rate * 1000) / LOOP_DELAY_MS));

      devices.emplace_back(device_uuid, unit_id, (poll_rate * 1000) / LOOP_DELAY_MS);
    }

    // calculate config MD5 chuecksum
    char* const payload_cpy = strdup(payload.c_str());
    unsigned char* const md5_hash = MD5::make_hash(payload_cpy);
    char* const md5_str = MD5::make_digest(md5_hash, 16);

    LOG(String("Config MD5 checksum: ") + md5_str);

    mqtt_client->publish_config_update(md5_str);

    free(payload_cpy);
    free(md5_hash);
    free(md5_str);
  } else if (topic.equals(String(MODULE_UUID) + "/SET_VALUE")) {

    const char* device_uuid = payload_json["device_uuid"];
    const char* datapoint = payload_json["datapoint"];
    const uint16_t value = payload_json["value"];    

    LOG("Setting value:");
    LOG(String("\t device_uuid: ") + device_uuid);
    LOG(String("\t datapoint: ") + datapoint);
    LOG(String("\t value: ") + value);
    
    // find the given device by its uuid and write the value according to datapoint
    for (H300& device : devices) {
      if (device.device_uuid == device_uuid) {
        if (String(datapoint).equals("SPEED")) {
          bool res = device.write_value(H300::speed_register, value);
          if (!res) {
            LOG("\t result: error");
          } else {
            LOG("\t result: ok");
          }
        } else if (String(datapoint).equals("SET_MOTION")) {
          bool res = device.write_value(H300::set_motion_register, value);
          if (!res) {
            LOG("\t result: error");
          } else {
            LOG("\t result: ok");
          }  
        } else {
          LOG("\tError: unrecognized datapoint");
        }

        break;   
      }   
    }
  } else if (topic.equals(String(MODULE_UUID) + "/UPDATE_FW")) {
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
