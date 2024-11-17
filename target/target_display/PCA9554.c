
#include "PCA9554.h"
#include "MyWire.h"
#include "MyUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PCA9554_ADDRESS 0x3F

#define PCA9554_INPUT_PORT_REG 0x00
#define PCA9554_OUTPUT_PORT_REG 0x01
#define PCA9554_INVERSION_PORT_REG 0x02
#define PCA9554_CONFIG_PORT_REG 0x03

#define PCA_TFT_RESET 2
#define PCA_TFT_CS 1
#define PCA_TFT_SCK 0
#define PCA_TFT_MOSI 7


void my_PCA9554_write_reg(uint8_t reg, uint8_t data);
void my_PCA9554_read_reg(uint8_t reg, uint8_t *data);
void my_PCA9554_begin_write_command(MY_PCA9554_HANDLE *handle);
void my_PCA9554_end_write_command(MY_PCA9554_HANDLE *handle);
void my_PCA9554_write_command(MY_PCA9554_HANDLE *handle, uint8_t command, uint8_t last_databit);


void my_PCA9554_create(MY_PCA9554_HANDLE *handle) {
    memset(handle, 0, sizeof(MY_PCA9554_HANDLE));
    handle->config_port = 0xff;
    handle->output_port = 0;

    int ret = my_wire_write(PCA9554_ADDRESS, NULL, 0);
    if (ret != 0) {
        printf("PCA9554 not found\n");
    } else {
        printf("PCA9554 found\n");

        my_PCA9554_write_reg(PCA9554_INVERSION_PORT_REG, 0); // no invert
        my_PCA9554_write_reg(PCA9554_CONFIG_PORT_REG, handle->config_port); // all input
        my_PCA9554_write_reg(PCA9554_OUTPUT_PORT_REG, handle->output_port); // all low

        my_PCA9554_pin_mode(handle, PCA_TFT_RESET, OUTPUT);
        my_PCA9554_digital_write(handle, PCA_TFT_RESET, LOW);
        my_delay(10);
        my_PCA9554_digital_write(handle, PCA_TFT_RESET, HIGH);
        my_delay(100);

        my_PCA9554_pin_mode(handle, PCA_TFT_CS, OUTPUT);
        my_PCA9554_digital_write(handle, PCA_TFT_CS, HIGH);
        my_PCA9554_pin_mode(handle, PCA_TFT_SCK, OUTPUT);
        my_PCA9554_digital_write(handle, PCA_TFT_SCK, HIGH);
        my_PCA9554_pin_mode(handle, PCA_TFT_MOSI, OUTPUT);
        my_PCA9554_digital_write(handle, PCA_TFT_MOSI, HIGH);
    }
}

void my_PCA9554_delete(MY_PCA9554_HANDLE *handle) {

}

void my_PCA9554_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buffer[2];
    buffer[0] = reg;
    buffer[1] = data;
    my_wire_write(PCA9554_ADDRESS, buffer, 2);
}

void my_PCA9554_read_reg(uint8_t reg, uint8_t *data) {
    my_wire_write(PCA9554_ADDRESS, &reg, 1);
    my_wire_read(PCA9554_ADDRESS, data, 1);
}

void my_PCA9554_pin_mode(MY_PCA9554_HANDLE *handle, uint8_t pin, uint8_t mode) {
    if (mode == OUTPUT) {
        handle->config_port &= ~(1UL << pin);
    } else {
        handle->config_port |= (1UL << pin);
    }
    my_PCA9554_write_reg(PCA9554_CONFIG_PORT_REG, handle->config_port);
}

void my_PCA9554_digital_write(MY_PCA9554_HANDLE *handle, uint8_t pin, uint8_t val) {
    if (val == HIGH) {
        handle->output_port |= (1UL << pin);
    } else {
        handle->output_port &= ~(1UL << pin);
    }
    my_PCA9554_write_reg(PCA9554_OUTPUT_PORT_REG, handle->output_port);
}

void my_PCA9554_begin_write_command(MY_PCA9554_HANDLE *handle) {
    my_PCA9554_digital_write(handle, PCA_TFT_CS, LOW);
}

void my_PCA9554_end_write_command(MY_PCA9554_HANDLE *handle) {
    my_PCA9554_digital_write(handle, PCA_TFT_CS, HIGH);
}

void my_PCA9554_write_command(MY_PCA9554_HANDLE *handle, uint8_t command, uint8_t last_databit) {
    my_PCA9554_digital_write(handle, PCA_TFT_MOSI, last_databit);
    my_PCA9554_digital_write(handle, PCA_TFT_SCK, LOW);
    my_PCA9554_digital_write(handle, PCA_TFT_SCK, HIGH);

    uint8_t bit = 0x80;
    while (bit) {
        if (command & bit) {
            if (last_databit != 1) {
                last_databit = 1;
                my_PCA9554_digital_write(handle, PCA_TFT_MOSI, HIGH);
            }
        } else {
            if (last_databit != 0) {
                last_databit = 0;
                my_PCA9554_digital_write(handle, PCA_TFT_MOSI, LOW);
            }
        }
        my_PCA9554_digital_write(handle, PCA_TFT_SCK, LOW);
        bit >>= 1;
        my_PCA9554_digital_write(handle, PCA_TFT_SCK, HIGH);
    }
}

void my_PCA9554_send_command(MY_PCA9554_HANDLE *handle, uint8_t command) {
    my_PCA9554_begin_write_command(handle);
    my_PCA9554_write_command(handle, command, 0);
    my_PCA9554_end_write_command(handle);
}

void my_PCA9554_batch(MY_PCA9554_HANDLE *handle, const uint8_t *operations, uint32_t len) {
    for (size_t i = 0; i < len; ++i) {
        switch (operations[i]) {
            case BEGIN_WRITE:
                my_PCA9554_begin_write_command(handle);
                break;
            case WRITE_C8_D8:
                my_PCA9554_write_command(handle, operations[++i], 0);
                my_PCA9554_write_command(handle, operations[++i], 1);
                break;
            case WRITE_C8_D16:
                my_PCA9554_write_command(handle, operations[++i], 0);
                my_PCA9554_write_command(handle, operations[++i], 0);
                my_PCA9554_write_command(handle, operations[++i], 1);
                break;
            case WRITE_COMMAND_8:
                my_PCA9554_write_command(handle, operations[++i], 0);
                break;
            case WRITE_BYTES:
                {
                    uint8_t count = operations[++i];
                    for (uint8_t j = 0; j < count; ++j) {
                        my_PCA9554_write_command(handle, operations[++i], j == count - 1);
                    }
                }
                break;
            case END_WRITE:
                my_PCA9554_end_write_command(handle);
                break;
            case DELAY:
                my_delay(operations[++i]);
                break;
            default:
                printf("Unknown operation id at %d: %d\n", i, operations[i]);
                break;
        }
    }
}

uint8_t my_PCA9554_digital_read(MY_PCA9554_HANDLE *handle, uint8_t pin) {
    uint8_t port;
    my_PCA9554_read_reg(PCA9554_INPUT_PORT_REG, &port);
    return (port & (1UL << pin)) ? HIGH : LOW;
}
