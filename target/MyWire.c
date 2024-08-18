
#include "MyWire.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "hal/i2c_ll.h"


#define I2C_BUS 0
#define I2C_BUFFER_LENGTH 128
#define FREQUENCY 1000000
#define SDA_PIN 8
#define SCL_PIN 18
#define TIMEOUT_MS 50


void my_wire_create(void) {
    printf("my_wire_create\n");

    printf("Initialising I2C Master: sda=%d scl=%d freq=%d\n", SDA_PIN, SCL_PIN, FREQUENCY);

    i2c_config_t conf = { };
    conf.mode = I2C_MODE_MASTER;
    conf.scl_io_num = (gpio_num_t)SCL_PIN;
    conf.sda_io_num = (gpio_num_t)SDA_PIN;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = FREQUENCY;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL; //Any one clock source that is available for the specified frequency may be choosen

    esp_err_t ret = i2c_param_config((i2c_port_t)I2C_BUS, &conf);
    if (ret != ESP_OK) {
        printf("i2c_param_config failed\n");
    }
    else {
        ret = i2c_driver_install((i2c_port_t)I2C_BUS, conf.mode, 0, 0, 0);
        if (ret != ESP_OK) {
            printf("i2c_driver_install failed\n");
        }
        else {
            //Clock Stretching Timeout: 20b:esp32, 5b:esp32-c3, 24b:esp32-s2
            i2c_set_timeout((i2c_port_t)I2C_BUS, I2C_LL_MAX_TIMEOUT);
        }
    }

    printf("MyWire ready!\n");
}

void my_wire_delete(void) {
    printf("my_wire_delete\n");
}

int my_wire_write(uint16_t address, uint8_t *data, uint32_t data_len) {
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = NULL;

    uint8_t cmd_buff[I2C_LINK_RECOMMENDED_SIZE(1)] = { 0 };

    cmd = i2c_cmd_link_create_static(cmd_buff, I2C_LINK_RECOMMENDED_SIZE(1));
    ret = i2c_master_start(cmd);
    if (ret != ESP_OK) {
        goto end;
    }
    ret = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    if (ret != ESP_OK) {
        goto end;
    }
    
    if (data_len > 0) {
        ret = i2c_master_write(cmd, data, data_len, true);
        if (ret != ESP_OK) {
            goto end;
        }
    }
    
    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK) {
        goto end;
    }
    ret = i2c_master_cmd_begin((i2c_port_t)I2C_BUS, cmd, TIMEOUT_MS / portTICK_PERIOD_MS);

end:
    if (cmd != NULL) {
        i2c_cmd_link_delete_static(cmd);
    }

    if (ret == ESP_OK) {
        return 0;
    }
    return 1;
}

int my_wire_read(uint16_t address, uint8_t *read_data, uint32_t read_data_len) {
    esp_err_t ret = i2c_master_read_from_device((i2c_port_t)I2C_BUS, address, read_data, read_data_len, TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret == ESP_OK) {
        return 0;
    }
    return 1;
}
