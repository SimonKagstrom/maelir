
#ifndef __MY_WIRE_H__
#define __MY_WIRE_H__


#include <inttypes.h>


void my_wire_create(void);
void my_wire_delete(void);
int my_wire_write(uint16_t address, uint8_t *data, uint32_t data_len);
int my_wire_read(uint16_t address, uint8_t *read_data, uint32_t read_data_len);


#endif
