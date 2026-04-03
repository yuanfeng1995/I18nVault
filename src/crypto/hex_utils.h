// Hex string parsing utilities
#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Convert a single hex character to its 0-15 value, or -1 on invalid input.
    int hex_nibble(char c);

    // Parse a hex string of exactly out_len*2 characters into out[].
    // Returns 0 on success, -1 on invalid input.
    int hex_parse(const char* hex, uint8_t* out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* HEX_UTILS_H */
