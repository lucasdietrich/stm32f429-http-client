#include "app_utils.h"

int tohex(char * dest, size_t len, uint8_t * buf, size_t buf_len)
{
    static const char table[] = "0123456789abcdef";

    int ret = -EINVAL;
    if (len >= buf_len << 1)
    {
        ret = buf_len << 1;

        for(size_t i = 0; i < buf_len; i++)
        {
            dest[2*i] = table[(uint8_t) (buf[i] >> 4) & 0xF];
            dest[2*i + 1] = table[(uint8_t) (buf[i] & 0xF)];
        }
    }
    return ret;
}

int fromhex(uint8_t * buf, size_t buf_len, char * input, size_t len)
{
    int ret = -EINVAL;
    uint8_t tmp = 0;
    if (buf_len << 1 >= len)
    {
        for (size_t i = 0; i < len; i++)
        {
            const char c = input[i];
            if ((c >= '0') && (c <= '9'))
            {
                tmp |= c - '0';
            }
            else if ((c >= 'a') && (c <= 'f'))
            {
                tmp |= 10u + c - 'a';
            }
            else
            {
                ret = -EINVAL;
                break;
            }

            if (i & 1)
            {
                buf[i >> 1] = tmp;
                tmp = 0;
            }
            else
            {
                tmp <<= 4;
            }
        }
        ret = len >> 1;
    }
    return ret;
}
