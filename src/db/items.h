/* item.h
   Rémi Attab (remi.attab@gmail.com), 20 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/types.h"
#include "utils/symbol.h"

struct atoms;
struct im_config;


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

#include "gen/item.h"

static_assert(sizeof(enum item) == 1);


typedef const enum item *im_list;
extern im_list im_list_control;
extern im_list im_list_factory;

const struct im_config *im_config(enum item item);
inline const struct im_config *im_config_assert(enum item item)
{
    const struct im_config *config = im_config(item);
    assert(config);
    return config;
}

void im_populate(void);
void im_populate_atoms(struct atoms *);


// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

inline bool item_validate(vm_word word)
{
    return word > 0 && word < items_max;
}

inline bool item_is_elem(enum item item)
{
    return item < items_synth_last;
}

inline bool item_is_natural(enum item item)
{
    return item >= items_natural_first && item < items_natural_last;
}

inline bool item_is_synth(enum item item)
{
    return item >= items_synth_first && item < items_synth_last;
}

inline bool item_is_passive(enum item item)
{
    return item >= items_passive_first && item < items_passive_last;
}

inline bool item_is_active(enum item item)
{
    return item >= items_active_first && item < items_active_last;
}

inline bool item_is_logistics(enum item item)
{
    return item >= items_logistics_first && item < items_logistics_last;
}


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

// Must match name_cap in gen/tech.c
enum { item_str_len = symbol_cap - 5 };

size_t item_str(enum item item, char *dst, size_t len);
const char *item_str_c(enum item item);


