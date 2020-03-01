#include "H300.hpp"
#include <stdint.h>
#include <Arduino.h>

H300::H300(const std::string device_id, const uint8_t unit_id, const uint16_t poll_rate)
  : device_id(device_id), unit_id(unit_id), poll_rate(poll_rate) 
{
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);

  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  // Modbus communication through hardware bus
  serial_bus.begin(baud_rate);
  node.begin(unit_id, serial_bus);

  // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(pre_transmission);
  node.postTransmission(post_transmission);
}

void H300::pre_transmission() {
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void H300::post_transmission() {
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

// Write value to holding register
bool H300::write_value(uint16_t register_addr, uint16_t value) {
  const uint8_t result = node.writeSingleRegister(register_addr, value);

  return result == node.ku8MBSuccess ? true : false;
}

// Read value from holding register
bool H300::read_value(uint16_t register_addr, uint16_t* response) {
  const uint8_t result = node.readHoldingRegisters(register_addr, 1);

  if (result != node.ku8MBSuccess) {
    return false;
  }

  *response = node.getResponseBuffer(0);
  return true;
}