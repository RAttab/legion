/* id.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "items/item.h"


// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

typedef uint32_t id_t;

inline id_t make_id(enum item type, id_t id) { return type << 24 | id; }
inline enum item id_item(id_t id) { return id >> 24; }
inline uint32_t id_bot(id_t id) { return id & ((1 << 24) - 1); }

inline bool id_validate(word_t word)
{
    return
        word > 0 &&
        word < UINT32_MAX &&
        item_validate(id_item(word));
}

enum { id_str_len = item_str_len+1+6 };
size_t id_str(id_t id, char *dst, size_t len);
