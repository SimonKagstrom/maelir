
#ifndef __MY_PCA9554_H__
#define __MY_PCA9554_H__


#include <inttypes.h>

#define LOW 0x0
#define HIGH 0x1

#define OUTPUT 0x03


typedef struct {
    uint8_t config_port;
    uint8_t output_port;
} MY_PCA9554_HANDLE;

typedef enum
{
  BEGIN_WRITE,
  WRITE_COMMAND_8,
  WRITE_C8_D8,
  WRITE_C8_D16,
  WRITE_BYTES,
  END_WRITE,
  DELAY,
} spi_operation_type_t;

void my_PCA9554_create(MY_PCA9554_HANDLE *handle);
void my_PCA9554_delete(MY_PCA9554_HANDLE *handle);
void my_PCA9554_pin_mode(MY_PCA9554_HANDLE *handle, uint8_t pin, uint8_t mode);
void my_PCA9554_digital_write(MY_PCA9554_HANDLE *handle, uint8_t pin, uint8_t val);
void my_PCA9554_send_command(MY_PCA9554_HANDLE *handle, uint8_t command);
void my_PCA9554_batch(MY_PCA9554_HANDLE *handle, const uint8_t *operations, uint32_t len);
uint8_t my_PCA9554_digital_read(MY_PCA9554_HANDLE *handle, uint8_t pin);


#endif
