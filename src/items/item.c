/* item.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/item.h"
#include "items/config.h"

// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

inline size_t item_str(enum item item, char *dst, size_t len)
{
    const struct im_config *config = im_config(item);

    len = legion_min(len-1, config->str_len);
    memcpy(dst, im_config(item)->str, len);
    dst[len] = 0;

    return len;
}

inline const char *item_str_c(enum item item)
{
    return im_config(item)->str;
}
