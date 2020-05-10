#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string>
#include <vector>
#include <map>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include <MD5.hpp>
#include "H300.hpp"

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

#define MODULE_TYPE  "VFD_H300"

#define LOOP_DELAY_MS   10u
#define FW_UPDATE_PORT  5000u

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL OBJECTS
////////////////////////////////////////////////////////////////////////////////

static String module_mac = WiFi.macAddress();

static FW_updater  *fw_updater = nullptr;
static MQTT_client *mqtt_client = nullptr;

static std::vector<H300> devices;

static bool standby_mode = false;

static void resolve_mqtt(String& topic, String& payload);

////////////////////////////////////////////////////////////////////////////////
/// SETUP
////////////////////////////////////////////////////////////////////////////////

void setup() 
{
  #if DEBUG == 1
    Serial.flush();
    Serial.end();
    delay(10);
    Serial.begin(115200);
  #endif

  LOG("Module MAC: " + module_mac);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);
  LOG("Connected to Wi-Fi AP");

  const String gateway_ip = WiFi.gatewayIP().toString();
  LOG("GW IP address: " + gateway_ip);

  if (fw_updater)
    delete fw_updater;

  // firmware server expected to run on GW
  fw_updater = new FW_updater(gateway_ip.c_str(), FW_UPDATE_PORT);
  
  if (mqtt_client)
    delete mqtt_client;
  
  // MQTT broker expected to run on GW
  mqtt_client = new MQTT_client(gateway_ip.c_str());
  LOG("Setting up MQTT client");
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

void loop() 
{
  if (!mqtt_client->loop())
  {
    LOG("Lost connection to MQTT broker, reconnecting...");
    setup();
  }

  // check if any device is present in config and standby mode is off
  if (devices.empty() || standby_mode) 
    return;

  // prepare json payload
  DynamicJsonDocument json(512);

  // loop through device vector
  for (H300& device : devices) 
  {
    // check if device is expected to be read (dependent on its poll rate)
    if (!device.decrease_counter())
      continue;
          
    LOG("Reading device: " + String(device.device_uuid.c_str()));

    JsonObject device_object = json.createNestedObject(device.device_uuid);

    uint16_t speed = 0;
    if (!device.read_value(H300::speed_register, &speed)) 
      device_object["SPEED"] = speed;
    
    uint16_t get_motion = 0;
    if (!device.read_value(H300::get_motion_register, &get_motion))
      device_object["GET_MOTION"] = get_motion;

    uint16_t state = 0;
    if (!device.read_value(H300::state_register, &state))
      device_object["STATE"] = state;

    uint16_t main_freq = 0;
    if (!device.read_value(H300::main_freq_register, &main_freq))
      device_object["MAIN_FREQUENCY"] = main_freq;

    uint16_t aux_freq = 0;
    if (!device.read_value(H300::aux_freq_register, &aux_freq))
      device_object["AUX_FREQUENCY"] = aux_freq;
    
    LOG(String("\tSPEED:\t") + speed);
    LOG(String("\tGET_MOTION:\t") + get_motion);
    LOG(String("\tSTATE:\t") + state);
    LOG(String("\tMAIN_FREQUENCY:\t") + main_freq);
    LOG(String("\tAUX_FREQUENCY:\t") + aux_freq);
  }

  // publish only if at least one device was read
  if (!json.isNull())
    mqtt_client->publish_value_update(json); 

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
        // stop all motors using DC breaks and switch to standy mode
        bool result = true;
        for (H300& device : devices)
        {
          const uint8_t res = device.write_value(H300::set_motion_register, 6);

          if (res != 0x00)
            result = false;
        }
          
        standby_mode = true;

        mqtt_client->publish_request_result(sequence_number, result);        
      } 
      else if (String(request) == "start") 
      {
        const uint16_t sequence_number = payload_json["sequence_number"];

        LOG("Switching to active mode");
        // switch to active mode
        standby_mode = false;

        mqtt_client->publish_request_result(sequence_number, true);
      }
    }
  } 
  else if (topic.equals(module_mac + "/SET_CONFIG")) 
  {    
    JsonObject json_config = payload_json.as<JsonObject>();
    LOG("Deleting previous configuration");
    std::vector<H300>().swap(devices); // delete previous configuration

    // create devices according to received configuration

    for (const JsonPair& pair : json_config) 
    { 
      const char* const device_uuid = pair.key().c_str();
      const JsonObject device_config = pair.value().as<JsonObject>();
      const uint8_t unit_id = device_config["address"];
      const uint16_t poll_rate = device_config["poll_rate"];

      LOG("Creating device with parameters: ");
      LOG(String("\t uuid:\t") + device_uuid);
      LOG(String("\t unit_id:\t") + unit_id);
      LOG(String("\t interval_rate:\t") + ((poll_rate * 1000) / LOOP_DELAY_MS));

      devices.emplace_back(device_uuid, unit_id, (poll_rate * 1000) / LOOP_DELAY_MS);
    }

    LOG(String("Actual device count: ") + devices.size());

    // calculate config MD5 chuecksum
    std::string payload_cpy(payload.c_str());
    const std::string& md5_str = MD5::make_digest(MD5::make_hash(&payload_cpy[0]), 16);

    LOG(String("Config MD5 checksum: ") + md5_str.c_str());

    mqtt_client->publish_config_update(md5_str);
  } 
  else if (topic.equals(module_mac + "/SET_VALUE")) 
  {
    const char* device_uuid = payload_json["device_uuid"];
    const char* datapoint = payload_json["datapoint"];
    const uint16_t value = payload_json["value"];    
    const uint16_t sequence_number = payload_json["sequence_number"];

    LOG("Setting value:");
    LOG(String("\t device_uuid: ") + device_uuid);
    LOG(String("\t datapoint: ") + datapoint);
    LOG(String("\t value: ") + value);
    
    // find the given device by its uuid and write the value according to datapoint
    for (const H300& device : devices) 
    {
      if (device.device_uuid == device_uuid) 
      {
        uint8_t result;

        if (String(datapoint).equals("SPEED")) 
        {
          result = device.write_value(H300::speed_register, value);        
        } 
        else if (String(datapoint).equals("SET_MOTION")) 
        {
          result = device.write_value(H300::set_motion_register, value);
        } 
        else
        {          
          const std::string error_msg("Error: unrecognized datapoint");
          LOG(String("\t") + error_msg.c_str());
          mqtt_client->publish_request_result(sequence_number, false, error_msg);

          break;
        }
          
        String log_msg = result == 0x00 ? "\t result: ok" : "\t result: error";
        LOG(log_msg);

        if (result == 0x00)
          mqtt_client->publish_request_result(sequence_number, true);
        else
        {
          const std::string error_msg("Error code: ");
          mqtt_client->publish_request_result(sequence_number, false, error_msg + String(result).c_str());
        }

        break;   
      }   
    }
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
