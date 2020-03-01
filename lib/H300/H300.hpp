#pragma once

#include <stdint.h>
#include <ModbusMaster.h>

class H300 {
  private:
    const std::string device_id;
    const uint8_t unit_id;
    const uint16_t poll_rate;
    ModbusMaster node;

    static constexpr unsigned long baud_rate = 9600;
    static constexpr HardwareSerial& serial_bus = Serial2;
    static constexpr uint8_t MAX485_DE = 18;
    static constexpr uint8_t MAX485_RE_NEG = 5;

    static void pre_transmission();
    static void post_transmission();
  public:
    static constexpr uint16_t speed_register = 0x1000;
    static constexpr uint16_t set_motion_register = 0x2000;
    static constexpr uint16_t get_motion_register = 0x3000;
    static constexpr uint16_t get_state_register = 0x8000;
    static constexpr uint16_t get_freq_setpoint_register = 0x101D;
    static constexpr uint16_t get_main_freq_register = 0x101F;
    static constexpr uint16_t get_aux_freq_register = 0x1020;

    H300(const std::string device_id, const uint8_t unit_id, const uint16_t poll_rate);
    bool write_value(uint16_t register_addr, uint16_t value);
    bool read_value(uint16_t register_addr, uint16_t* response);
};