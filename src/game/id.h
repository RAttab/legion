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

typedef uint16_t id;
enum { id_shift = 8 };

inline id make_id(enum item type, id id) { return type << id_shift | id; }
inline enum item id_item(id id) { return id >> id_shift; }
inline uint32_t id_bot(id id) { return id & ((1 << id_shift) - 1); }

inline bool id_validate(vm_word word)
{
    return
        word > 0 &&
        word < UINT16_MAX &&
        item_validate(id_item(word));
}

enum { id_str_len = item_str_len + 1 + 3 };
size_t id_str(id id, char *dst, size_t len);
