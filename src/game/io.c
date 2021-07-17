/* io.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "game/io.h"
#include "game/item.h"
#include "vm/vm.h"


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

static void atom_io_reg(
        struct atoms *atoms, word_t val, const char *str, size_t len)
{
    assert(len <= symbol_cap);
    struct symbol symbol = make_symbol_len(str, len);
    assert(atoms_set(atoms, &symbol, val));
}

void atoms_io_register(struct atoms *atoms)
{
#define reg_atom(atom) atoms_reg(atoms, atom, #atom, sizeof(#atom))

    // IO
    reg_atom(IO_NIL);
    reg_atom(IO_PING);
    reg_atom(IO_PONG);
    reg_atom(IO_RESET);
    reg_atom(IO_ITEM);
    reg_atom(IO_PROG);
    reg_atom(IO_MOD);
    reg_atom(IO_GET);
    reg_atom(IO_SET);
    reg_atom(IO_VAL);
    reg_atom(IO_SEND);
    reg_atom(IO_RECV);
    reg_atom(IO_SCAN);
    reg_atom(IO_RESULT);
    reg_atom(IO_LAUNCH);

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
