/* atoms.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "game/atoms.h"
#include "game/item.h"
#include "vm/vm.h"


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

static void atoms_reg(unsigned val, const char *str, size_t len)
{
    assert(len <= vm_atom_cap);

    atom_t atom = {0};
    memcpy(atom, str, len);

    for (size_t i = 0; i < vm_atom_cap; ++i)
        atom[i] = tolower(atom[i]);

    assert(vm_atoms_set(&atom, val));
}

void atoms_register(void)
{
#define reg_ele(atom) atoms_reg(ITEM_ ## atom, #atom, sizeof(#atom))
    reg_ele(ELEM_A);
    reg_ele(ELEM_B);
    reg_ele(ELEM_C);
    reg_ele(ELEM_D);
    reg_ele(ELEM_E);
    reg_ele(ELEM_F);
    reg_ele(ELEM_G);
    reg_ele(ELEM_H);
    reg_ele(ELEM_I);
    reg_ele(ELEM_J);
    reg_ele(ELEM_K);
    reg_ele(ELEM_L);
    reg_ele(ELEM_M);
    reg_ele(ELEM_N);
    reg_ele(ELEM_O);
    reg_ele(ELEM_P);

    reg_ele(ELEM_Q);
    reg_ele(ELEM_R);
    reg_ele(ELEM_S);
    reg_ele(ELEM_T);
    reg_ele(ELEM_U);
    reg_ele(ELEM_V);
    reg_ele(ELEM_W);
    reg_ele(ELEM_X);
    reg_ele(ELEM_Y);
    reg_ele(ELEM_Z);
#undef reg_ele

#define reg_atom(atom) atoms_reg(atom, #atom, sizeof(#atom))
    reg_atom(ITEM_WORKER);
    reg_atom(ITEM_PRINTER);
    reg_atom(ITEM_MINER);
    reg_atom(ITEM_DEPLOYER);

    reg_atom(ITEM_BRAIN_S);
    reg_atom(ITEM_BRAIN_M);
    reg_atom(ITEM_BRAIN_L);
    reg_atom(ITEM_DB_S);
    reg_atom(ITEM_DB_M);
    reg_atom(ITEM_DB_L);

    reg_atom(IO_NIL);

    reg_atom(IO_PING);
    reg_atom(IO_PONG);

    reg_atom(IO_RESET);
    reg_atom(IO_PROG);

    reg_atom(IO_GET);
    reg_atom(IO_SET);
    reg_atom(IO_VAL);

    reg_atom(IO_SEND);
    reg_atom(IO_RECV);
#undef reg_atom
}
