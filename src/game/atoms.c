/* atoms.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "atoms.h"
#include "vm/vm.h"
#include "game/item.h"

// -----------------------------------------------------------------------------
// atoms_io
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
    reg_ele(ELE_A);
    reg_ele(ELE_B);
    reg_ele(ELE_C);
    reg_ele(ELE_D);
    reg_ele(ELE_E);
    reg_ele(ELE_F);
    reg_ele(ELE_G);
    reg_ele(ELE_H);
    reg_ele(ELE_I);
    reg_ele(ELE_J);
    reg_ele(ELE_K);
    reg_ele(ELE_L);
    reg_ele(ELE_M);
    reg_ele(ELE_N);
    reg_ele(ELE_O);
    reg_ele(ELE_P);

    reg_ele(ELE_Q);
    reg_ele(ELE_R);
    reg_ele(ELE_S);
    reg_ele(ELE_T);
    reg_ele(ELE_U);
    reg_ele(ELE_V);
    reg_ele(ELE_W);
    reg_ele(ELE_X);
    reg_ele(ELE_Y);
    reg_ele(ELE_Z);
#undef reg_ele

#define reg_atom(atom) atoms_reg(atom, #atom, sizeof(#atom))
    reg_atom(ITEM_BRAIN);
    reg_atom(ITEM_WORKER);
    reg_atom(ITEM_PRINTER);
    reg_atom(ITEM_LAB);
    reg_atom(ITEM_COMM);
    reg_atom(ITEM_SHIP);

    reg_atom(IO_NOOP);
    reg_atom(IO_OK);
    reg_atom(IO_FAIL);

    reg_atom(IO_ID);
    reg_atom(IO_TARGET);
    reg_atom(IO_SEND);
    reg_atom(IO_SENDN);
    reg_atom(IO_RECV);
    reg_atom(IO_RECVN);
    reg_atom(IO_CARGO);
    reg_atom(IO_VENT);

    reg_atom(IO_DOCK);
    reg_atom(IO_UNDOCK);
    reg_atom(IO_TAKE);
    reg_atom(IO_PUT);
    reg_atom(IO_HARVEST);
#undef reg_atom
}
