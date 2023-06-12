/* io.c
   Rémi Attab (remi.attab@gmail.com), 20 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "io.h"
#include "vm/atoms.h"
#include "utils/symbol.h"

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

struct io_config
{
    vm_word word;
    const char *atom;
    size_t atom_len;

};

#define io_register(_io, _atom, _len)           \
    [_io - io_min] = (struct io_config) {       \
        .word = _io,                            \
        .atom = _atom,                          \
        .atom_len = _len,                       \
    }

#define ioe_register(_ioe, _atom, _len)                 \
    [(_ioe - ioe_min) + io_len] = (struct io_config) {  \
        .word = _ioe,                                   \
        .atom = _atom,                                  \
        .atom_len = _len,                               \
    }

static struct io_config io_configs[io_len + ioe_len] =
{
    #include "gen/io_register.h"
};

#undef io_register
#undef ioe_register

void io_populate_atoms(struct atoms *atoms)
{
    for (size_t i = 0; i < array_len(io_configs); ++i) {
        struct io_config *config = &io_configs[i];
        if (!config->word) continue;

        struct symbol symbol = make_symbol_len(config->atom, config->atom_len);
        bool ok = atoms_set(atoms, &symbol, config->word);
        assert(ok);
    }
}
