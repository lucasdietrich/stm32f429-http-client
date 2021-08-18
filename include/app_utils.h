#ifndef _APP_UTILS_H_
#define _APP_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include <errno.h>

int u32_to_hex(uint32_t number, char * const output, size_t len);

int hex_to_u8_buf(char * const input, uint8_t* buffer, size_t len);


#endif