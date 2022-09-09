/* id.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/id.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

inline size_t im_id_str(im_id id, char *base, size_t len)
{
    assert(len >= im_id_str_len);
    char *it = base;
    const char *end = base + len;

    it += item_str(im_id_item(id), it, end - it);
    *it = '.'; it++;
    it += str_utox(im_id_seq(id), it, 2);

    if (it != end) *it = 0;
    return it - base;
}
