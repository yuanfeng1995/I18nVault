#include "hex_utils.h"

#include <string.h>

int hex_nibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

int hex_parse(const char* hex, uint8_t* out, size_t out_len)
{
    if (!hex)
        return -1;

    size_t hex_len = strlen(hex);
    if (hex_len != out_len * 2)
        return -1;

    for (size_t i = 0; i < out_len; ++i)
    {
        int hi = hex_nibble(hex[2 * i]);
        int lo = hex_nibble(hex[2 * i + 1]);
        if (hi < 0 || lo < 0)
            return -1;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}
