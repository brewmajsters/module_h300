#include "FW_updater.hpp"

FW_updater::FW_updater(const char gw_ip[], const unsigned short port)
{
  httpUpdate.setLedPin(2, LOW);

  asprintf(&url, "http://%s:%u/module_firmware/", gw_ip, port);
}

bool FW_updater::update(const char version[])
{
  t_httpUpdate_return ret = httpUpdate.update(wifi_client, strcat(url, version));

  return ret != HTTP_UPDATE_OK ? true : false;
}
