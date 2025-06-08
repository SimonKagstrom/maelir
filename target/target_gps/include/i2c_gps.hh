#pragma once

#include "hal/i_gps.hh"
#include "nmea_parser.hh"

#include <array>
#include <driver/i2c_master.h>

class I2cGps : public hal::IGps
{
public:
    I2cGps(uint8_t scl_pin, uint8_t sda_pin);

private:
    std::optional<hal::RawGpsData> WaitForData(os::binary_semaphore& semaphore) final;

    i2c_master_bus_handle_t m_bus_handle;
    i2c_master_dev_handle_t m_dev_handle;

    std::array<char, 256> m_line_buf;

    std::unique_ptr<NmeaParser> m_parser;
};
