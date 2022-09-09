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

typedef uint16_t im_id;
enum { id_shift = 8 };

inline im_id make_im_id(enum item type, im_id id) { return type << id_shift | id; }
inline enum item im_id_item(im_id id) { return id >> id_shift; }
inline uint32_t im_id_seq(im_id id) { return id & ((1 << id_shift) - 1); }

inline bool id_validate(vm_word word)
{
    return
        word > 0 &&
        word < UINT16_MAX &&
        item_validate(im_id_item(word));
}

enum { im_id_str_len = item_str_len + 1 + 3 };
size_t im_id_str(im_id id, char *dst, size_t len);
