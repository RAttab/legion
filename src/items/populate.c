/* populate.c
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/atoms.h"
#include "items/io.h"
#include "items/item.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// im_config
// -----------------------------------------------------------------------------

#define im_init_cfg(_type, _str, _cfg)          \
    [_type] = (struct im_config) {              \
        .type = _type,                          \
        .str = _str,                            \
        .atom = #_type,                         \
        .config = _cfg                          \
    }

#define im_init(_type_, _str_) im_init_cfg(_type_, _str_, NULL)


static struct im_config im_configs[ITEM_MAX] =
{
    im_init(ITEM_NIL, "nil"),

    // Elements
    im_init(ITEM_ELEM_A, "elem.A"),
    im_init(ITEM_ELEM_B, "elem.B"),
    im_init(ITEM_ELEM_C, "elem.C"),
    im_init(ITEM_ELEM_D, "elem.D"),
    im_init(ITEM_ELEM_E, "elem.E"),
    im_init(ITEM_ELEM_F, "elem.F"),
    im_init(ITEM_ELEM_G, "elem.G"),
    im_init(ITEM_ELEM_H, "elem.H"),
    im_init(ITEM_ELEM_I, "elem.I"),
    im_init(ITEM_ELEM_J, "elem.J"),
    im_init(ITEM_ELEM_K, "elem.K"),
    im_init(ITEM_ELEM_L, "elem.L"),
    im_init(ITEM_ELEM_M, "elem.M"),
    im_init(ITEM_ELEM_N, "elem.N"),
    im_init(ITEM_ELEM_O, "elem.O"),
    im_init(ITEM_ELEM_P, "elem.P"),
    im_init(ITEM_ELEM_Q, "elem.Q"),
    im_init(ITEM_ELEM_R, "elem.R"),
    im_init(ITEM_ELEM_S, "elem.S"),
    im_init(ITEM_ELEM_T, "elem.T"),
    im_init(ITEM_ELEM_U, "elem.U"),
    im_init(ITEM_ELEM_V, "elem.V"),
    im_init(ITEM_ELEM_W, "elem.W"),
    im_init(ITEM_ELEM_X, "elem.X"),
    im_init(ITEM_ELEM_Y, "elem.Y"),
    im_init(ITEM_ELEM_Z, "elem.Z"),

    // Passive
    im_init(ITEM_FRAME,        "frame"),
    im_init(ITEM_LOGIC,        "logic"),
    im_init(ITEM_NEURON,       "neuron"),
    im_init(ITEM_BOND,         "bond"),
    im_init(ITEM_MAGNET,       "magnet"),
    im_init(ITEM_NUCLEAR,      "nuclear"),
    im_init(ITEM_ROBOTICS,     "robotics"),
    im_init(ITEM_CORE,         "core"),
    im_init(ITEM_CAPACITOR,    "capacitor"),
    im_init(ITEM_MATRIX,       "matrix"),
    im_init(ITEM_MAGNET_FIELD, "magnet_field"),
    im_init(ITEM_HULL,         "hull"),

    // Active
    im_init(ITEM_DEPLOY, "deploy", im_deploy_config),

    // Logistics
};

#undef im_init
#undef im_init_cfg


const struct im_config *im_config(enum item item)
{
    assert(item >= 0 && item < ITEM_MAX);
    assert(!item || im_config[item].type);
    return &im_config[item];
}


// -----------------------------------------------------------------------------
// io_config
// -----------------------------------------------------------------------------

struct io_config
{
    enum io io;
    const char *atom;
};

#define io_init(_io)                            \
    [_io - IO_MIN] = (struct io_config) {       \
        .io = _io,                              \
        .atom = #_io,                           \
    }

static io_config io_configs[IO_LEN] =
{
    // Generic
    io_init(IO_NIL),
    io_init(IO_OK),
    io_init(IO_FAIL),
    io_init(IO_PING),
    io_init(IO_PONG),
    io_init(IO_STATUS),
    io_init(IO_STATE),
    io_init(IO_RESET),
    io_init(IO_ITEM),
    io_init(IO_TAPE),
    io_init(IO_MOD),

    // Brain
    io_init(IO_COORD),
    io_init(IO_SEND),
    io_init(IO_RECV),
    io_init(IO_DBG_ATTACH),
    io_init(IO_DBG_DETACH),
    io_init(IO_DBG_BREAK),
    io_init(IO_DBG_STEP),

    // Misc
    io_init(IO_GET),
    io_init(IO_SET),
    io_init(IO_VAL),
    io_init(IO_SCAN),
    io_init(IO_SCAN_VAL),
    io_init(IO_LAUNCH),
    io_init(IO_LEARN),
    io_init(IO_TAPE_DATA),
    io_init(IO_TAPE_AT),
};

#undef io_init


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

static void populate_atom(struct atoms *atoms, const char *str, word_t value)
{
    if (!atoms) return;

    size_t len = strlen(str);
    assert(len <= symbol_cap);

    struct symbol = make_symbol_len(str, len);
    for (size_t i = 0; i < symbol.len; ++i) {
        char c = tolower(symbol.c[i]);
        if (c) symbol.c[i] = c;
    }

    bool ok = atoms_set(atoms, &symbol, item);
    assert(ok);
}

void im_populate(struct atoms *atoms)
{
    for (size_t i = 0; i < ITEM_MAX; ++i) {
        struct im_config *config = &im_configs[i];
        if (i && !config->type) continue;

        config->str_len = strlen(config->str);
        assert(str_len < item_str_len);

        if (config->init) config->init(config);
        populate_atom(atoms, config->atom, config->type);
    }

    for (size_t i = 0; i < IO_MAX; ++i) {
        struct io_config *config = &io_configs[i];
        if (!config->io) continue;

        populate_atom(atoms, config->atom, config->io);
    }
}
