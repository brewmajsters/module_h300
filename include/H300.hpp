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
    const std::string device_id;
    const uint8_t unit_id;
    const uint32_t poll_rate;
    
    static constexpr uint16_t speed_register = 		0x1000; // writable
    static constexpr uint16_t state_register = 		0x8000;
    static constexpr uint16_t get_freq_register =	0x1001;
    static constexpr uint16_t set_freq_register = 	0xF00C;	// writable
    static constexpr uint16_t get_motion_register = 	0x3000;
    static constexpr uint16_t set_motion_register = 	0x2000;	// writable
    static constexpr uint16_t accel_time_register = 	0xF011;	// writable
    static constexpr uint16_t decel_time_register = 	0xF012;	// writable
    static constexpr uint16_t get_timer_register = 	0x1015;
    static constexpr uint16_t set_timer_register =	0xF82C; // writable
    
    H300(const std::string device_id, const uint8_t unit_id, const uint32_t poll_rate);
    uint8_t write_value(const uint16_t register_addr, const uint16_t value) const;
    uint8_t read_value(const uint16_t register_addr, uint16_t* const response) const;
    bool decrease_counter();
};
