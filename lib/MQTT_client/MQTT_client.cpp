#include "MQTT_client.hpp"

MQTT_client::MQTT_client(const char* gw_ip, const uint32_t port, const uint16_t buffer_size) 
  : MQTTClient(buffer_size)
{
  begin(gw_ip, port, wifi_client);
  setOptions(5, true, 1000);
}

void MQTT_client::setup_mqtt(
  const std::string& module_mac, 
  const std::string& module_type, 
  MQTTClientCallbackSimple callback
) {
  this->module_mac = module_mac;
  this->module_type = module_type;
  
  char lw_msg[256];
  StaticJsonDocument<JSON_OBJECT_SIZE(1) + 150> json;
  json["module_mac"] = module_mac;
  serializeJson(json, lw_msg);

  onMessage(callback);
  setWill("MODULE_DISCONNECT", lw_msg, false, 2);

  while (!connect(module_mac.c_str(), false)) {
    delay(1000);
  }
}

bool MQTT_client::publish_module_id(const uint8_t QOS) {
  char msg[256];
  StaticJsonDocument<JSON_OBJECT_SIZE(2) + 256> json;
  json["module_mac"] = module_mac;
  json["module_type"] = module_type;
  serializeJson(json, msg);

  return publish("MODULE_ID", msg, false, QOS);
}

bool MQTT_client::publish_config_update(const std::string& config_hash, const uint8_t QOS) {
  char msg[256];
  StaticJsonDocument<JSON_OBJECT_SIZE(2) + 256> json;
  json["module_mac"] = module_mac;
  json["config_hash"] = config_hash;
  serializeJson(json, msg);

  return publish("MODULE_CONFIG_UPDATE", msg, false, QOS);
}

bool MQTT_client::publish_value_update(DynamicJsonDocument values_json, const uint8_t QOS) {
  char msg[256];
  DynamicJsonDocument json(512);
  json["module_mac"] = module_mac;
  json["values"] = values_json;
  serializeJson(json, msg);

  return publish("VALUE_UPDATE", msg, false, QOS);
}

MQTT_client::~MQTT_client() {
  disconnect();
}