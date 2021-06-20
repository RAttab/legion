/* str.h
   Rémi Attab (remi.attab@gmail.com), 09 Dec 2020
   FreeBSD-style copyright and disclaimer apply

   \todo need to write meself a proper string library
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

size_t str_utoa(uint64_t val, char *dst, size_t len);
size_t str_utox(uint64_t val, char *dst, size_t len);

enum { str_scaled_len = 4 };
size_t str_scaled(uint64_t val, char *dst, size_t len);

inline char str_hexchar(uint8_t val)
{
    val &= 0xF;
    return val < 0xA ? '0' + val : 'A' + (val - 0xA);
}

inline uint8_t str_charhex(char val)
{
    if (val >= '0' && val <= '9') return val - '0';
    if (val >= 'A' && val <= 'F') return val - 'A';
    if (val >= 'a' && val <= 'f') return val - 'a';
    return 0xFF;
}

char str_keycode_shift(unsigned char c);

