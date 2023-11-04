/* items.c
   Rémi Attab (remi.attab@gmail.com), 19 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/vm.h"
#include "game/game.h"
#include "items/config.h"
#include "items/items.h"

// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

#define im_register_cfg(_type, _str, _str_len, _atom, _cfg)     \
    [_type] = (struct im_config) {                              \
    .type = _type,                                              \
    .str = _str,                                                \
    .str_len = _str_len,                                        \
    .atom = _atom,                                              \
    .init = _cfg,                                               \
    }

#define im_register(_type, _str, _str_len, _atom)       \
    im_register_cfg(_type, _str,_str_len, _atom, NULL)

static struct im_config im_configs[items_max] =
{
    #include "gen/im_register.h"
};

#undef im_register
#undef im_register_cfg

const struct im_config *im_config(enum item item)
{
    assert(item < items_max);
    return &im_configs[item];
}

void im_populate(void)
{
    for (size_t i = 1; i < items_max; ++i) {
        struct im_config *config = im_configs + i;
        assert(config->str_len <= item_str_len);
        if (config->init) config->init(config);
    }
}

void im_populate_atoms(struct atoms *atoms)
{
    // !item_nil can't be registered as it has the value 0
    for (size_t i = 1; i < items_max; ++i) {
        struct im_config *config = im_configs + i;
        struct symbol atom = make_symbol(config->atom);
        bool ok = atoms_set(atoms, &atom, config->type);
        assert(ok);
    }
}

// -----------------------------------------------------------------------------
// list
// -----------------------------------------------------------------------------

static const enum item im_list_control_arr[] =
{
    #include "gen/im_control.h"
    0,
};
im_list im_list_control = im_list_control_arr;


static const enum item im_list_factory_arr[] =
{
    #include "gen/im_factory.h"
    0,
};
im_list im_list_factory = im_list_factory_arr;


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

inline size_t item_str(enum item item, char *dst, size_t len)
{
    const struct im_config *config = im_config_assert(item);

    len = legion_min(len-1, config->str_len);
    memcpy(dst, im_config(item)->str, len);
    dst[len] = 0;

    return len;
}

inline const char *item_str_c(enum item item)
{
    return im_config(item)->str;
}
