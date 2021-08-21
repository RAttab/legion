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
#include "items/db/db.h"
#include "items/deploy/deploy.h"
#include "items/extract/extract.h"
#include "items/lab/lab.h"
#include "items/legion/legion.h"
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

    // Passive
    im_init(ITEM_FRAME,      "frame",      bits_s, work_s),
    im_init(ITEM_GEAR,       "gear",       bits_s, work_s),
    im_init(ITEM_FUEL,       "fuel",       bits_s, work_s),
    im_init(ITEM_BONDING,    "bonding",    bits_s, work_s),
    im_init(ITEM_CIRCUIT,    "circuit",    bits_s, work_s),
    im_init(ITEM_NEURAL,     "neural",     bits_s, work_s),
    im_init(ITEM_SERVO,      "servo",      bits_s, work_s),
    im_init(ITEM_THRUSTER,   "thruster",   bits_s, work_s),
    im_init(ITEM_PROPULSION, "propulsion", bits_s, work_s),
    im_init(ITEM_PLATE,      "plate",      bits_s, work_s),
    im_init(ITEM_SHIELDING,  "shielding",  bits_s, work_s),
    im_init(ITEM_HULL_1,     "hull_1",     bits_s, work_s),
    im_init(ITEM_CORE,       "core",       bits_s, work_s),
    im_init(ITEM_MATRIX,     "matrix",     bits_s, work_s),
    im_init(ITEM_DATABANK,   "databank",   bits_s, work_s),

    // Active
    im_init_cfg(ITEM_DEPLOY,     "deploy",     bits_s, work_s, im_deploy_config),
    im_init_cfg(ITEM_EXTRACT_1,  "extract_1",  bits_s, work_s, im_extract_config),
    im_init_cfg(ITEM_EXTRACT_2,  "extract_2",  bits_s, work_s, im_extract_config),
    im_init_cfg(ITEM_EXTRACT_3,  "extract_3",  bits_s, work_s, im_extract_config),
    im_init_cfg(ITEM_PRINTER_1,  "printer_1",  bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_PRINTER_2,  "printer_2",  bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_PRINTER_3,  "printer_3",  bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_ASSEMBLY_1, "assembly_1", bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_ASSEMBLY_2, "assembly_2", bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_ASSEMBLY_3, "assembly_3", bits_s, work_s, im_printer_config),
    im_init_cfg(ITEM_STORAGE,    "storage",    bits_s, work_s, im_storage_config),
    im_init_cfg(ITEM_SCANNER_1,  "scanner_1",  bits_s, work_s, im_scanner_config),
    im_init_cfg(ITEM_SCANNER_2,  "scanner_2",  bits_s, work_s, im_scanner_config),
    im_init_cfg(ITEM_SCANNER_3,  "scanner_3",  bits_s, work_s, im_scanner_config),
    im_init_cfg(ITEM_LAB,        "lab",        bits_s, work_s, im_lab_config),
    im_init_cfg(ITEM_DB_1,       "db_1",       bits_s, work_s, im_db_config),
    im_init_cfg(ITEM_DB_2,       "db_2",       bits_s, work_s, im_db_config),
    im_init_cfg(ITEM_DB_3,       "db_3",       bits_s, work_s, im_db_config),
    im_init_cfg(ITEM_BRAIN_1,    "brain_1",    bits_s, work_s, im_brain_config),
    im_init_cfg(ITEM_BRAIN_2,    "brain_2",    bits_s, work_s, im_brain_config),
    im_init_cfg(ITEM_BRAIN_3,    "brain_3",    bits_s, work_s, im_brain_config),
    im_init_cfg(ITEM_LEGION_1,   "legion_1",   bits_s, work_s, im_legion_config),
    im_init_cfg(ITEM_LEGION_2,   "legion_2",   bits_s, work_s, im_legion_config),
    im_init_cfg(ITEM_LEGION_3,   "legion_3",   bits_s, work_s, im_legion_config),

    // Logistics
    im_init(ITEM_WORKER, "worker", bits_s, work_s),
    im_init(ITEM_BULLET, "bullet", bits_s, work_s),
};

#undef im_init
#undef im_init_cfg


const enum item im_list_control_arr[] =
{
    ITEM_SCANNER_1, ITEM_SCANNER_2, ITEM_SCANNER_3,
    ITEM_DB_1, ITEM_DB_2, ITEM_DB_3,
    ITEM_BRAIN_1, ITEM_BRAIN_2, ITEM_BRAIN_3,
    ITEM_LEGION_1, ITEM_LEGION_2, ITEM_LEGION_3,
    0,
};
im_list_t im_list_control = im_list_control_arr;


const enum item im_list_factory_arr[] =
{
    ITEM_DEPLOY, ITEM_STORAGE,
    ITEM_EXTRACT_1, ITEM_EXTRACT_2, ITEM_EXTRACT_3,
    ITEM_PRINTER_1, ITEM_PRINTER_2, ITEM_PRINTER_3,
    ITEM_ASSEMBLY_1, ITEM_ASSEMBLY_2, ITEM_ASSEMBLY_3,
    ITEM_LAB,
    0,
};
im_list_t im_list_factory = im_list_factory_arr;


const enum item im_list_t0_arr[] =
{
    ITEM_FRAME,
    ITEM_GEAR,
    ITEM_FUEL,
    ITEM_BONDING,
    ITEM_CIRCUIT,
    ITEM_NEURAL,
    ITEM_SERVO,
    ITEM_THRUSTER,
    ITEM_PROPULSION,
    ITEM_PLATE,
    ITEM_SHIELDING,
    ITEM_HULL_1,
    ITEM_CORE,
    ITEM_MATRIX,
    ITEM_DATABANK,

    ITEM_DEPLOY,
    ITEM_STORAGE,
    ITEM_EXTRACT_1,
    ITEM_PRINTER_1,
    ITEM_ASSEMBLY_1,
    ITEM_LAB,
    ITEM_SCANNER_1,
    ITEM_DB_1,
    ITEM_BRAIN_1,
    ITEM_LEGION_1,

    0,
};
im_list_t im_list_t0 = im_list_t0_arr;


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
