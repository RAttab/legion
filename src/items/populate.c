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
#include "items/legion/legion.h"
#include "items/printer/printer.h"
#include "items/research/research.h"
#include "items/scanner/scanner.h"
#include "items/storage/storage.h"


// -----------------------------------------------------------------------------
// im_config
// -----------------------------------------------------------------------------

#define im_init_cfg(_type, _str, _cfg)          \
    [_type] = (struct im_config) {              \
        .type = _type,                          \
        .str = _str,                            \
        .atom = #_type,                         \
        .init = _cfg,                           \
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
    im_init(ITEM_FRAME,      "frame"),
    im_init(ITEM_GEAR,       "gear"),
    im_init(ITEM_FUEL,       "fuel"),
    im_init(ITEM_BONDING,    "bonding"),
    im_init(ITEM_CIRCUIT,    "circuit"),
    im_init(ITEM_NEURAL,     "neural"),
    im_init(ITEM_SERVO,      "servo"),
    im_init(ITEM_THRUSTER,   "thruster"),
    im_init(ITEM_PROPULSION, "propulsion"),
    im_init(ITEM_PLATE,      "plate"),
    im_init(ITEM_SHIELDING,  "shielding"),
    im_init(ITEM_HULL_1,     "hull_1"),
    im_init(ITEM_CORE,       "core"),
    im_init(ITEM_MATRIX,     "matrix"),
    im_init(ITEM_DATABANK,   "databank"),

    // Active
    im_init_cfg(ITEM_DEPLOY,     "deploy", im_deploy_config),
    im_init_cfg(ITEM_EXTRACT_1,  "extract_1", im_extract_config),
    im_init_cfg(ITEM_EXTRACT_2,  "extract_2", im_extract_config),
    im_init_cfg(ITEM_EXTRACT_3,  "extract_3", im_extract_config),
    im_init_cfg(ITEM_PRINTER_1,  "printer_1", im_printer_config),
    im_init_cfg(ITEM_PRINTER_2,  "printer_2", im_printer_config),
    im_init_cfg(ITEM_PRINTER_3,  "printer_3", im_printer_config),
    im_init_cfg(ITEM_ASSEMBLY_1, "assembly_1", im_printer_config),
    im_init_cfg(ITEM_ASSEMBLY_2, "assembly_2", im_printer_config),
    im_init_cfg(ITEM_ASSEMBLY_3, "assembly_3", im_printer_config),
    im_init_cfg(ITEM_STORAGE,    "storage", im_storage_config),
    im_init_cfg(ITEM_SCANNER_1,  "scanner_1", im_scanner_config),
    im_init_cfg(ITEM_SCANNER_2,  "scanner_2", im_scanner_config),
    im_init_cfg(ITEM_SCANNER_3,  "scanner_3", im_scanner_config),
    im_init_cfg(ITEM_RESEARCH,   "research", im_research_config),
    im_init_cfg(ITEM_DB_1,       "db_1", im_db_config),
    im_init_cfg(ITEM_DB_2,       "db_2", im_db_config),
    im_init_cfg(ITEM_DB_3,       "db_3", im_db_config),
    im_init_cfg(ITEM_BRAIN_1,    "brain_1", im_brain_config),
    im_init_cfg(ITEM_BRAIN_2,    "brain_2", im_brain_config),
    im_init_cfg(ITEM_BRAIN_3,    "brain_3", im_brain_config),
    im_init_cfg(ITEM_LEGION_1,   "legion_1", im_legion_config),
    im_init_cfg(ITEM_LEGION_2,   "legion_2", im_legion_config),
    im_init_cfg(ITEM_LEGION_3,   "legion_3", im_legion_config),

    // Logistics
    im_init(ITEM_WORKER, "worker"),
    im_init(ITEM_BULLET, "bullet"),

};

#undef im_init
#undef im_init_cfg


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
