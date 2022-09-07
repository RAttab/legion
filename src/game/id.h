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

typedef uint16_t id_t;
enum { id_shift = 8 };

inline id_t make_id(enum item type, id_t id) { return type << id_shift | id; }
inline enum item id_item(id_t id) { return id >> id_shift; }
inline uint32_t id_bot(id_t id) { return id & ((1 << id_shift) - 1); }

inline bool id_validate(word_t word)
{
    return
        word > 0 &&
        word < UINT16_MAX &&
        item_validate(id_item(word));
}

enum { id_str_len = item_str_len + 1 + 3 };
size_t id_str(id_t id, char *dst, size_t len);
