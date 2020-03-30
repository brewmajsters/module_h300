#pragma once

#include <stdint.h>
#include <ModbusMaster.h>

class H300 
{
  private:
    mutable ModbusMaster node;

    static constexpr unsigned long baud_rate = 19200;
    static constexpr HardwareSerial& serial_bus = Serial2;
    static constexpr uint8_t MAX485_DE = 18;
    static constexpr uint8_t MAX485_RE_NEG = 5;

    static void pre_transmission();
    static void post_transmission();

    uint32_t iteration_counter;
  public:
    const std::string device_uuid;
    const uint8_t unit_id;
    const uint32_t poll_rate;
    
    static constexpr uint16_t speed_register = 0x1000;
    static constexpr uint16_t set_motion_register = 0x2000;
    static constexpr uint16_t get_motion_register = 0x3000;
    static constexpr uint16_t state_register = 0x8000;
    static constexpr uint16_t main_freq_register = 0x101F;
    static constexpr uint16_t aux_freq_register = 0x1020;

    H300(const std::string device_uuid, const uint8_t unit_id, const uint32_t poll_rate);
    uint8_t write_value(const uint16_t register_addr, const uint16_t value) const;
    uint8_t read_value(const uint16_t register_addr, uint16_t* const response) const;
    bool decrease_counter();
};