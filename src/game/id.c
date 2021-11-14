/* id.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/id.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

inline size_t id_str(id_t id, char *base, size_t len)
{
    assert(len >= id_str_len);
    char *it = base;
    const char *end = base + len;

    it += str_utox(id_bot(id), it, 6);
    *it = '.'; it++;
    it += item_str(id_item(id), it, end - it);
    *it = 0;

    return it - base;
}
