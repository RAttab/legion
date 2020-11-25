/* atoms.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#define atom_str(a) [a - ATOM_IO_MIN] = #a,

static const char *atoms_io_table[] = {
    atom_str(io_noop),

    atom_str(io_id),
    atom_str(io_target),
    atom_str(io_send),
    atom_str(io_sendn),
    atom_str(io_recv),
    atom_str(io_recvn),
    atom_str(io_cargo),
    atom_str(io_dump),
};

#undef atom_str

const char *atoms_io_str(enum atoms_io atom)
{
    return atoms_io_table[atom - ATOM_IO_MIN];
}
