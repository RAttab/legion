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

inline char str_hexchar(uint8_t val)
{
    val &= 0xF;
    return val < 0xA ? '0' + val : 'A' + (val - 0xA);
}
