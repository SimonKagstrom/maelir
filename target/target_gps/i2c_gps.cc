#include "i2c_gps.hh"

I2cGps::I2cGps(uint8_t scl_pin, uint8_t sda_pin)
{
    const i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = static_cast<gpio_num_t>(sda_pin),
        .scl_io_num = static_cast<gpio_num_t>(scl_pin),
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags {
            .enable_internal_pullup = true,
        },
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &m_bus_handle));
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x10,
        .scl_speed_hz = 100000,
    };


    ESP_ERROR_CHECK(i2c_master_bus_add_device(m_bus_handle, &dev_cfg, &m_dev_handle));

    constexpr auto kAllData = "PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0";
    constexpr auto kOneSecondUpdateRate = "PMTK220,1000";

    // Configure the device
    ESP_ERROR_CHECK(i2c_master_transmit(
        m_dev_handle, reinterpret_cast<const uint8_t*>(kAllData), sizeof(kAllData), 1000));
    ESP_ERROR_CHECK(i2c_master_transmit(m_dev_handle,
                                        reinterpret_cast<const uint8_t*>(kOneSecondUpdateRate),
                                        sizeof(kOneSecondUpdateRate),
                                        1000));
}

std::optional<hal::RawGpsData>
I2cGps::WaitForData(os::binary_semaphore& semaphore)
{
    std::optional<hal::RawGpsData> data;

    memset(m_line_buf.data(), 0, m_line_buf.size());
    auto err = i2c_master_transmit_receive(m_dev_handle,
                                           reinterpret_cast<uint8_t*>(m_line_buf.data()),
                                           m_line_buf.size(),
                                           reinterpret_cast<uint8_t*>(m_line_buf.data()),
                                           m_line_buf.size(),
                                           -1);

    if (err == ESP_OK)
    {
        if (m_line_buf[0] != ' ')
        {
            printf("XXX: %s\n", m_line_buf.data());
        }
        //        data = m_parser->PushData(std::string_view(m_line_buf.data(), m_line_buf.size()));
    }
    os::Sleep(100ms);
    semaphore.release();

    return data;
}
