/* populate.c
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/atoms.h"
#include "items/io.h"
#include "db/items.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// io_config
// -----------------------------------------------------------------------------

struct io_config
{
    vm_word word;
    const char *atom;
};

#define io_init(_io)                            \
    [_io - IO_MIN] = (struct io_config) {       \
        .word = _io,                            \
        .atom = #_io,                           \
    }

#define ioe_init(_ioe)                                  \
    [(_ioe - IOE_MIN) + IO_LEN] = (struct io_config) {  \
        .word = _ioe,                                   \
        .atom = #_ioe,                                  \
    }

static struct io_config io_configs[IO_LEN + IOE_LEN] =
{
    // Generic
    io_init(IO_NIL),
    io_init(IO_OK),
    io_init(IO_FAIL),
    io_init(IO_STEP),
    io_init(IO_ARRIVE),
    io_init(IO_RETURN),
    io_init(IO_PING),
    io_init(IO_PONG),
    io_init(IO_STATE),
    io_init(IO_RESET),
    io_init(IO_ITEM),
    io_init(IO_TAPE),
    io_init(IO_MOD),
    io_init(IO_LOOP),

    // Brain
    io_init(IO_ID),
    io_init(IO_TICK),
    io_init(IO_COORD),
    io_init(IO_LOG),
    io_init(IO_NAME),
    io_init(IO_SEND),
    io_init(IO_RECV),
    io_init(IO_DBG_ATTACH),
    io_init(IO_DBG_DETACH),
    io_init(IO_DBG_BREAK),
    io_init(IO_DBG_STEP),
    io_init(IO_SPECS),

    // Lab
    io_init(IO_TAPE_AT),
    io_init(IO_TAPE_KNOWN),
    io_init(IO_ITEM_BITS),
    io_init(IO_ITEM_KNOWN),

    // Tx/Rx
    io_init(IO_CHANNEL),
    io_init(IO_TRANSMIT),
    io_init(IO_RECEIVE),
    // Nomad
    io_init(IO_PACK),
    io_init(IO_LOAD),
    io_init(IO_UNLOAD),

    // Misc
    io_init(IO_GET),
    io_init(IO_SET),
    io_init(IO_CAS),
    io_init(IO_PROBE),
    io_init(IO_SCAN),
    io_init(IO_VALUE),
    io_init(IO_LAUNCH),
    io_init(IO_TARGET),
    io_init(IO_GROW),
    io_init(IO_INPUT),
    io_init(IO_ACTIVATE),

    // State
    io_init(IO_HAS_ITEM),
    io_init(IO_HAS_LOOP),
    io_init(IO_SIZE),
    io_init(IO_RATE),
    io_init(IO_WORK),
    io_init(IO_OUTPUT),
    io_init(IO_CARGO),
    io_init(IO_ENERGY),
    io_init(IO_ACTIVE),

    // Errors
    ioe_init(IOE_MISSING_ARG),
    ioe_init(IOE_INVALID_STATE),
    ioe_init(IOE_VM_FAULT),
    ioe_init(IOE_STARVED),
    ioe_init(IOE_OUT_OF_RANGE),
    ioe_init(IOE_OUT_OF_SPACE),
    ioe_init(IOE_INVALID_SPEC),
    ioe_init(IOE_A0_INVALID),
    ioe_init(IOE_A0_UNKNOWN),
    ioe_init(IOE_A1_INVALID),
    ioe_init(IOE_A1_UNKNOWN),
};

#undef io_init


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

static void populate_atom(struct atoms *atoms, const char *str, vm_word value)
{
    if (!atoms) return;

    size_t len = strlen(str);
    assert(len <= symbol_cap);

    struct symbol symbol = make_symbol_len(str, len);
    for (size_t i = 0; i < symbol.len; ++i) {
        char c = tolower(symbol.c[i]);
        if (c == '_') c = '-';
        if (c) symbol.c[i] = c;
    }

    bool ok = atoms_set(atoms, &symbol, value);
    assert(ok);
}

void io_populate_atoms(struct atoms *atoms)
{
    for (size_t i = 0; i < array_len(io_configs); ++i) {
        struct io_config *config = &io_configs[i];
        if (!config->word) continue;

        populate_atom(atoms, config->atom, config->word);
    }
}
