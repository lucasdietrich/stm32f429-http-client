#ifndef _APP_UTILS_H_
#define _APP_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include <errno.h>

int tohex(char * dest, size_t len, uint8_t * buf, size_t buf_len);

int fromhex(uint8_t * buf, size_t buf_len, char * input, size_t len);


#endif