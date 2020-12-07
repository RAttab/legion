/* atoms.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "atoms.h"
#include "vm/vm.h"

// -----------------------------------------------------------------------------
// atoms_io
// -----------------------------------------------------------------------------

static void atoms_io_reg(enum atom_io val, const char *str, size_t len)
{
    assert(len <= vm_atom_cap);

    atom_t atom = {0};
    memcpy(atom, str, len);

    for (size_t i = 0; i < vm_atom_cap; ++i)
        atom[i] = tolower(atom[i]);

    assert(vm_atoms_set(&atom, val));
}

void atoms_io_register(void)
{
#define atom_reg(atom) atoms_io_reg(atom, #atom, sizeof(#atom))
    atom_reg(IO_NOOP);
    atom_reg(IO_OK);
    atom_reg(IO_FAIL);

    atom_reg(IO_ID);
    atom_reg(IO_TARGET);
    atom_reg(IO_SEND);
    atom_reg(IO_SENDN);
    atom_reg(IO_RECV);
    atom_reg(IO_RECVN);
    atom_reg(IO_CARGO);
    atom_reg(IO_VENT);

    atom_reg(IO_DOCK);
    atom_reg(IO_UNDOCK);
    atom_reg(IO_TAKE);
    atom_reg(IO_PUT);
    atom_reg(IO_HARVEST);
#undef atom_reg
}
