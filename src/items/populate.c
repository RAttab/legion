/* populate.c
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/atoms.h"
#include "items/io.h"
#include "items/item.h"
#include "items/config.h"

#include "items/brain/brain.h"
#include "items/deploy/deploy.h"
#include "items/extract/extract.h"
#include "items/lab/lab.h"
#include "items/legion/legion.h"
#include "items/memory/memory.h"
#include "items/printer/printer.h"
#include "items/scanner/scanner.h"
#include "items/storage/storage.h"


// -----------------------------------------------------------------------------
// im_config
// -----------------------------------------------------------------------------

#define im_init_cfg(_type, _str, _bits, _work, _cfg)    \
    [_type] = (struct im_config) {                      \
        .type = _type,                                  \
        .str = _str,                                    \
        .atom = #_type,                                 \
        .init = _cfg,                                   \
        .lab_bits = _bits,                              \
        .lab_work = _work,                              \
    }

#define im_init(_type_, _str, _bits, _work)     \
    im_init_cfg(_type_, _str, _bits, _work, NULL)


enum { bits_s = 8, bits_m = 16, bits_l = 32, bits_xl = 64 };
enum { work_s = 8, work_m = 16, work_l = 32, work_xl = 64 };

static struct im_config im_configs[ITEM_MAX] =
{
    im_init(ITEM_NIL, "nil", bits_s, work_s),

    // Elements
    im_init(ITEM_ELEM_A, "elem.A", bits_s, work_s),
    im_init(ITEM_ELEM_B, "elem.B", bits_s, work_s),
    im_init(ITEM_ELEM_C, "elem.C", bits_s, work_s),
    im_init(ITEM_ELEM_D, "elem.D", bits_s, work_s),
    im_init(ITEM_ELEM_E, "elem.E", bits_s, work_s),
    im_init(ITEM_ELEM_F, "elem.F", bits_s, work_s),
    im_init(ITEM_ELEM_G, "elem.G", bits_s, work_s),
    im_init(ITEM_ELEM_H, "elem.H", bits_s, work_s),
    im_init(ITEM_ELEM_I, "elem.I", bits_s, work_s),
    im_init(ITEM_ELEM_J, "elem.J", bits_s, work_s),
    im_init(ITEM_ELEM_K, "elem.K", bits_s, work_s),
    im_init(ITEM_ELEM_L, "elem.L", bits_s, work_s),
    im_init(ITEM_ELEM_M, "elem.M", bits_s, work_s),
    im_init(ITEM_ELEM_N, "elem.N", bits_s, work_s),
    im_init(ITEM_ELEM_O, "elem.O", bits_s, work_s),
    im_init(ITEM_ELEM_P, "elem.P", bits_s, work_s),
    im_init(ITEM_ELEM_Q, "elem.Q", bits_s, work_s),
    im_init(ITEM_ELEM_R, "elem.R", bits_s, work_s),
    im_init(ITEM_ELEM_S, "elem.S", bits_s, work_s),
    im_init(ITEM_ELEM_T, "elem.T", bits_s, work_s),
    im_init(ITEM_ELEM_U, "elem.U", bits_s, work_s),
    im_init(ITEM_ELEM_V, "elem.V", bits_s, work_s),
    im_init(ITEM_ELEM_W, "elem.W", bits_s, work_s),
    im_init(ITEM_ELEM_X, "elem.X", bits_s, work_s),
    im_init(ITEM_ELEM_Y, "elem.Y", bits_s, work_s),
    im_init(ITEM_ELEM_Z, "elem.Z", bits_s, work_s),

    // Passive - T0
    im_init(ITEM_FRAME,        "frame",        bits_s, work_s),
    im_init(ITEM_LOGIC,        "logic",        bits_s, work_s),
    im_init(ITEM_GEAR,         "gear",         bits_s, work_s),
    im_init(ITEM_NEURON,       "neuron",       bits_s, work_s),
    im_init(ITEM_BOND,         "bond",         bits_s, work_s),
    im_init(ITEM_MAGNET,       "magnet",       bits_s, work_s),
    im_init(ITEM_NUCLEAR,      "nuclear",      bits_s, work_s),
    im_init(ITEM_ROBOTICS,     "robotics",     bits_s, work_s),
    im_init(ITEM_CORE,         "core",         bits_s, work_s),
    im_init(ITEM_CAPACITOR,    "capacitor",    bits_s, work_s),
    im_init(ITEM_MATRIX,       "matrix",       bits_s, work_s),
    im_init(ITEM_MAGNET_FIELD, "magnet-field", bits_s, work_s),
    im_init(ITEM_HULL,         "hull",         bits_s, work_s),
    // Passive - T1
    im_init(ITEM_CAPTOR,       "captor",       bits_m, work_m),
    im_init(ITEM_LIQUID_FRAME, "liquid_frame", bits_m, work_m),
    im_init(ITEM_RADIATION,    "radiation",    bits_m, work_m),
    im_init(ITEM_ANTENNA,      "antenna",      bits_m, work_m),
    im_init(ITEM_ACCELERATOR,  "accelerator",  bits_m, work_m),

    // Active - T0
    im_init_cfg(ITEM_DEPLOY,       "deploy",    bits_s, work_s, im_deploy_config),
    im_init_cfg(ITEM_EXTRACT,      "extract",   bits_s, work_s, im_extract_config),
    im_init_cfg(ITEM_PRINTER,      "printer",   bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_ASSEMBLY,     "assembly",  bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_MEMORY,       "memory",    bits_s, work_s, im_memory_config),
    im_init_cfg(ITEM_BRAIN,        "brain",     bits_s, work_s, im_brain_config),
    im_init_cfg(ITEM_SCANNER,      "scanner",   bits_s, work_s, im_scanner_config),
    im_init_cfg(ITEM_LEGION,       "legion",    bits_s, work_s, im_legion_config),
    im_init_cfg(ITEM_LAB,          "lab",       bits_s, work_s, im_lab_config),
    // Active - T1
    im_init_cfg(ITEM_STORAGE,      "storage",   bits_m, work_m, im_storage_config),
    /* im_init_cfg(ITEM_PORT,         "port",      bits_m, work_m, im_port_config), */
    im_init_cfg(ITEM_CONDENSER,    "condenser", bits_m, work_m, im_extract_config),
    /* im_init_cfg(ITEM_AUTO_DEPLOY,  "auto-deploy",      bits_m, work_m, im_auto_deploy_config), */
    /* im_init_cfg(ITEM_TRANSMIT,     "transmit",  bits_m, work_m, im_transmit_config), */
    /* im_init_cfg(ITEM_RECEIVE,      "receive",   bits_m, work_m, im_receive_config), */

    // Logistics
    im_init(ITEM_WORKER,       "worker",       bits_s, work_s),
    im_init(ITEM_BULLET,       "bullet",       bits_s, work_s),
    im_init(ITEM_SOLAR,        "solar",        bits_s, work_s),
    im_init(ITEM_KWHEEL,       "k-wheel",      bits_s, work_s),
    im_init(ITEM_ENERGY_STORE, "energy-store", bits_s, work_s),
};

#undef im_init
#undef im_init_cfg


const enum item im_list_control_arr[] =
{
    ITEM_MEMORY,
    ITEM_BRAIN,
    ITEM_SCANNER,
    ITEM_LEGION,
    0,
};
im_list_t im_list_control = im_list_control_arr;


const enum item im_list_factory_arr[] =
{
    ITEM_DEPLOY,
    ITEM_EXTRACT,
    ITEM_PRINTER,
    ITEM_ASSEMBLY,
    ITEM_LAB,
    0,
};
im_list_t im_list_factory = im_list_factory_arr;


const struct im_config *im_config(enum item item)
{
    assert(item < ITEM_MAX);
    if (item && !im_configs[item].type) return NULL;
    return &im_configs[item];
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

static struct io_config io_configs[IO_LEN] =
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

    // Lab
    io_init(IO_TAPE_AT),
    io_init(IO_TAPE_KNOWN),
    io_init(IO_ITEM_BITS),
    io_init(IO_ITEM_KNOWN),

    // Misc
    io_init(IO_GET),
    io_init(IO_SET),
    io_init(IO_VAL),
    io_init(IO_SCAN),
    io_init(IO_SCAN_VAL),
    io_init(IO_LAUNCH),
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

    struct symbol symbol = make_symbol_len(str, len);
    for (size_t i = 0; i < symbol.len; ++i) {
        char c = tolower(symbol.c[i]);
        if (c) symbol.c[i] = c;
    }

    bool ok = atoms_set(atoms, &symbol, value);
    assert(ok);
}

void im_populate(void)
{
    for (size_t i = 0; i < ITEM_MAX; ++i) {
        struct im_config *config = &im_configs[i];
        if (i && !config->type) continue;

        config->str_len = strlen(config->str);
        assert(config->str_len < item_str_len);

        if (config->init) config->init(config);
    }
}

void im_populate_atoms(struct atoms *atoms)
{
    // !item_nil can't be registered as it has the value 0
    for (size_t i = 1; i < ITEM_MAX; ++i) {
        struct im_config *config = &im_configs[i];
        if (i && !config->type) continue;

        populate_atom(atoms, config->atom, config->type);
    }

    for (size_t i = 0; i < IO_LEN; ++i) {
        struct io_config *config = &io_configs[i];
        if (!config->io) continue;

        populate_atom(atoms, config->atom, config->io);
    }
}
