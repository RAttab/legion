/* str.c
   Rémi Attab (remi.attab@gmail.com), 09 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "str.h"


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

size_t str_utoa(uint64_t val, char *dst, size_t len)
{
    size_t i = 0;
    for (; i < len; ++i, val /= 10)
        dst[len-i-1] = '0' + (val % 10);
    return i;
}

size_t str_utox(uint64_t val, char *dst, size_t len)
{
    size_t i = 0;
    for (; i < len; ++i, val >>= 4)
        dst[len-i-1] = str_hexchar(val);
    return i;
}
