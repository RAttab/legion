/* item.c
   Rémi Attab (remi.attab@gmail.com), 06 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/item.h"
#include "game/io.h"
#include "vm/atoms.h"
#include "vm/symbol.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

inline size_t id_str(id_t id, char *base, size_t len)
{
    assert(len <= id_str_len);
    char *it = base;
    char *end = base + len;

    it += str_utox(id_bot(id), it, 6);
    *it = '.'; it++;
    it += item_str(id_item(id), it, end - it);
    *it = 0;

    return it - base;
}


// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------

static const char *item_str_table[] =
{
    [ITEM_NIL] = "null",

    // Natural
    [ITEM_ELEM_A] = "elem.A",
    [ITEM_ELEM_B] = "elem.B",
    [ITEM_ELEM_C] = "elem.C",
    [ITEM_ELEM_D] = "elem.D",
    [ITEM_ELEM_E] = "elem.E",
    [ITEM_ELEM_F] = "elem.F",
    [ITEM_ELEM_G] = "elem.G",
    [ITEM_ELEM_H] = "elem.H",
    [ITEM_ELEM_I] = "elem.I",
    [ITEM_ELEM_J] = "elem.J",
    [ITEM_ELEM_K] = "elem.K",

    // Synth
    [ITEM_ELEM_L] = "elem.L",
    [ITEM_ELEM_M] = "elem.M",
    [ITEM_ELEM_N] = "elem.N",
    [ITEM_ELEM_O] = "elem.O",
    [ITEM_ELEM_P] = "elem.P",
    [ITEM_ELEM_Q] = "elem.Q",
    [ITEM_ELEM_R] = "elem.R",
    [ITEM_ELEM_S] = "elem.S",
    [ITEM_ELEM_T] = "elem.T",
    [ITEM_ELEM_U] = "elem.U",
    [ITEM_ELEM_V] = "elem.V",
    [ITEM_ELEM_W] = "elem.W",
    [ITEM_ELEM_X] = "elem.X",
    [ITEM_ELEM_Y] = "elem.Y",
    [ITEM_ELEM_Z] = "elem.Z",

    // Passives
    [ITEM_FRAME]      = "frame",
    [ITEM_GEAR]       = "gear",
    [ITEM_FUEL]       = "fuel",
    [ITEM_BONDING]    = "bonding",
    [ITEM_CIRCUIT]    = "circuit",
    [ITEM_NEURAL]     = "neural",
    [ITEM_SERVO]      = "servo",
    [ITEM_THRUSTER]   = "thruster",
    [ITEM_PROPULSION] = "propulsion",
    [ITEM_PLATE]      = "plate",
    [ITEM_SHIELDING]  = "shield",
    [ITEM_HULL_1]     = "hull",
    [ITEM_CORE]       = "core",
    [ITEM_MATRIX]     = "matrix",
    [ITEM_DATABANK]   = "databank",

    // Logistics
    [ITEM_WORKER]    = "worker",
    [ITEM_SHUTTLE_1] = "shuttle.1",
    [ITEM_SHUTTLE_2] = "shuttle.2",
    [ITEM_SHUTTLE_3] = "shuttle.3",

    // Actives
    [ITEM_DEPLOY]      = "deploy",
    [ITEM_EXTRACT_1]   = "extract.1",
    [ITEM_EXTRACT_2]   = "extract.2",
    [ITEM_EXTRACT_3]   = "extract.3",
    [ITEM_PRINTER_1]   = "printer.1",
    [ITEM_PRINTER_2]   = "printer.2",
    [ITEM_PRINTER_3]   = "printer.3",
    [ITEM_ASSEMBLY_1]  = "assembly.1",
    [ITEM_ASSEMBLY_2]  = "assembly.2",
    [ITEM_ASSEMBLY_3]  = "assembly.3",
    [ITEM_STORAGE]     = "storage",
    [ITEM_SCANNER_1]   = "scanner.1",
    [ITEM_SCANNER_2]   = "scanner.2",
    [ITEM_SCANNER_3]   = "scanner.3",
    [ITEM_DB_1]        = "db.1",
    [ITEM_DB_2]        = "db.2",
    [ITEM_DB_3]        = "db.3",
    [ITEM_BRAIN_1]     = "brain.1",
    [ITEM_BRAIN_2]     = "brain.2",
    [ITEM_BRAIN_3]     = "brain.3",
    [ITEM_LEGION_1]    = "legion.1",
    [ITEM_LEGION_2]    = "legion.2",
    [ITEM_LEGION_3]    = "legion.3",

    0,
};

const char *item_str_c(enum item item)
{
    assert(item < array_len(item_str_table));
    return item_str_table[item];
}

size_t item_str(enum item item, char *dst, size_t cap)
{
    assert(cap >= item_str_len);

    const char *str = item_str_c(item);
    assert(str);

    size_t len = strlen(str);
    assert(len < item_str_len);

    memcpy(dst, str, len);
    dst[len] = 0;
    return len;
}


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

static void atom_game_reg(
        struct atoms *atoms, word_t val, const char *str, size_t len)
{
    assert(len <= symbol_cap);
    struct symbol symbol = make_symbol_len(str, len);
    for (size_t i = 0; i < symbol.len; ++i) {
        char c = tolower(symbol.c[i]);
        if (c) symbol.c[i] = c;
    }
    assert(atoms_set(atoms, &symbol, val));
}

void atoms_register_game(struct atoms *atoms)
{
#define reg_atom(atom) atom_game_reg(atoms, atom, #atom, sizeof(#atom))

    // IO
    reg_atom(IO_NIL);
    reg_atom(IO_OK);
    reg_atom(IO_FAIL);
    reg_atom(IO_PING);
    reg_atom(IO_PONG);
    reg_atom(IO_STATUS);
    reg_atom(IO_STATE);
    reg_atom(IO_COORD);
    reg_atom(IO_RESET);
    reg_atom(IO_ITEM);
    reg_atom(IO_TAPE);
    reg_atom(IO_MOD);
    reg_atom(IO_GET);
    reg_atom(IO_SET);
    reg_atom(IO_VAL);
    reg_atom(IO_SEND);
    reg_atom(IO_RECV);
    reg_atom(IO_SCAN);
    reg_atom(IO_SCAN_VAL);
    reg_atom(IO_LAUNCH);
    reg_atom(IO_DBG_ATTACH);
    reg_atom(IO_DBG_DETACH);
    reg_atom(IO_DBG_BREAK);
    reg_atom(IO_DBG_STEP);

    // Natural
    reg_atom(ITEM_ELEM_A);
    reg_atom(ITEM_ELEM_B);
    reg_atom(ITEM_ELEM_C);
    reg_atom(ITEM_ELEM_D);
    reg_atom(ITEM_ELEM_E);
    reg_atom(ITEM_ELEM_F);
    reg_atom(ITEM_ELEM_G);
    reg_atom(ITEM_ELEM_H);
    reg_atom(ITEM_ELEM_I);
    reg_atom(ITEM_ELEM_J);
    reg_atom(ITEM_ELEM_K);

    // Synth
    reg_atom(ITEM_ELEM_L);
    reg_atom(ITEM_ELEM_M);
    reg_atom(ITEM_ELEM_N);
    reg_atom(ITEM_ELEM_O);
    reg_atom(ITEM_ELEM_P);
    reg_atom(ITEM_ELEM_Q);
    reg_atom(ITEM_ELEM_R);
    reg_atom(ITEM_ELEM_S);
    reg_atom(ITEM_ELEM_T);
    reg_atom(ITEM_ELEM_U);
    reg_atom(ITEM_ELEM_V);
    reg_atom(ITEM_ELEM_W);
    reg_atom(ITEM_ELEM_X);
    reg_atom(ITEM_ELEM_Y);
    reg_atom(ITEM_ELEM_Z);

    // Passives
    reg_atom(ITEM_FRAME);
    reg_atom(ITEM_GEAR);
    reg_atom(ITEM_FUEL);
    reg_atom(ITEM_BONDING);
    reg_atom(ITEM_CIRCUIT);
    reg_atom(ITEM_NEURAL);
    reg_atom(ITEM_SERVO);
    reg_atom(ITEM_THRUSTER);
    reg_atom(ITEM_PROPULSION);
    reg_atom(ITEM_PLATE);
    reg_atom(ITEM_SHIELDING);
    reg_atom(ITEM_HULL_1);
    reg_atom(ITEM_CORE);
    reg_atom(ITEM_MATRIX);
    reg_atom(ITEM_DATABANK);

    // Logistics
    reg_atom(ITEM_WORKER);
    reg_atom(ITEM_SHUTTLE_1);
    reg_atom(ITEM_SHUTTLE_2);
    reg_atom(ITEM_SHUTTLE_3);

    // Actives
    reg_atom(ITEM_DEPLOY);
    reg_atom(ITEM_EXTRACT_1);
    reg_atom(ITEM_EXTRACT_2);
    reg_atom(ITEM_EXTRACT_3);
    reg_atom(ITEM_PRINTER_1);
    reg_atom(ITEM_PRINTER_2);
    reg_atom(ITEM_PRINTER_3);
    reg_atom(ITEM_ASSEMBLY_1);
    reg_atom(ITEM_ASSEMBLY_2);
    reg_atom(ITEM_ASSEMBLY_3);
    reg_atom(ITEM_STORAGE);
    reg_atom(ITEM_SCANNER_1);
    reg_atom(ITEM_SCANNER_2);
    reg_atom(ITEM_SCANNER_3);
    reg_atom(ITEM_DB_1);
    reg_atom(ITEM_DB_2);
    reg_atom(ITEM_DB_3);
    reg_atom(ITEM_BRAIN_1);
    reg_atom(ITEM_BRAIN_2);
    reg_atom(ITEM_BRAIN_3);
    reg_atom(ITEM_LEGION_1);
    reg_atom(ITEM_LEGION_2);
    reg_atom(ITEM_LEGION_3);

#undef reg_atom
}
