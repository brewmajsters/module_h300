#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <FW_updater.hpp>
#include <MQTT_client.hpp>
#include <MD5.hpp>
#include <H300.hpp>

static const char* ssid     = "ssid";
static const char* password = "password";

static const char* module_id   = "DUMMY_ID";
static const char* module_type = "DUMMY_TYPE";

static const uint32_t loop_delay = 10;

static FW_updater  *fw_updater = nullptr;
static MQTT_client *mqtt_client = nullptr;

void resolve_mqtt(String& topic, String& payload);

void setup(){
  Serial.begin(115200);
  delay(10);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
  }

  char* gateway_ip = nullptr;
  IPAddress gw = WiFi.gatewayIP();
  asprintf(&gateway_ip, "%d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);

  fw_updater = new FW_updater(gateway_ip, 5000);

  mqtt_client = new MQTT_client(gateway_ip);
  mqtt_client->set_mqtt_params(module_id, module_type, resolve_mqtt);
  mqtt_client->connect();
  mqtt_client->publish_module_id();
  mqtt_client->subscribe("ALL_MODULES");
  mqtt_client->subscribe((std::string(module_id) + "/SET_CONFIG").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/SET_VALUE").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/UPDATE_FW").c_str());
  mqtt_client->subscribe((std::string(module_id) + "/REQUEST").c_str());
}

void loop(){
  mqtt_client->mqtt_loop();

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

  delay(loop_delay);
}

void resolve_mqtt(String& topic, String& payload){
  Serial.println("incoming: " + topic + " - " + payload);

  DynamicJsonDocument payload_json(256);
  DeserializationError json_err = deserializeJson(payload_json, payload);

  if (json_err) {
    Serial.println("JSON error: code " + String(json_err.c_str()));
    return;
  }

  if (topic.equals("ALL_MODULES") || topic.equals(String(module_id) + "/REQUEST")) {
    const char* request = payload_json["request"];

    if (request != nullptr){
      if (String(request) == "module_discovery"){
        mqtt_client->publish_module_id();
      } else if (String(request) == "shutdown"){
        // TODO: CUSTOM SHUTDOWN ACTION
      } else if (String(request) == "init") {
        // TODO: CUSTOM INIT ACTION
      }
    }
  } else if (topic.equals(String(module_id) + "/SET_CONFIG")){
    JsonObject json_config = payload_json.as<JsonObject>();

    for (JsonPair pair : json_config) {
      const char* device_id = pair.key().c_str();
      const auto device_config = pair.value().as<JsonObject>();

      // TODO: CUSTOM DEVICE ADDRESS ASSIGNMENT (ACCORDING TO DATATYPE AND RANGE)
      // E.G: const uint8_t address = device_config["address"];

      const uint16_t poll_rate = device_config["poll_rate"];

      // TODO: CUSTOM DEVICE CREATION ACTION
    }

    char* payload_cpy = strdup(payload.c_str());
    unsigned char* md5_hash = MD5::make_hash(payload_cpy);
    char* md5_str = MD5::make_digest(md5_hash, 16);
    mqtt_client->publish_config_update(md5_str);

    free(payload_cpy);
    free(md5_hash);
    free(md5_str);
  } else if (topic.equals(String(module_id) + "/SET_VALUE")){
    const uint32_t device_id = payload_json["device_id"];
    const char* datapoint = payload_json["datapoint"];
    // TODO: CUSTOM VALUE ASSIGNMENT (ACCORDING TO DATATYPE AND RANGE)
    // E.G: const uint8_t value = payload_json["value"];

    Serial.print("device_id: " + device_id);
    Serial.print(" datapoint: " + String(datapoint));
    // Serial.println(" value: " + String(value));

    // TODO: CUSTOM SET_VALUE ACTION
  } else if (topic.equals(String(module_id) + "/UPDATE_FW")){
    const char* version = payload_json["version"];

    fw_updater->update(version);
  }
}
