/* atoms.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// atoms_io
// -----------------------------------------------------------------------------

enum atom_io
{
    ATOM_IO_MIN = 0x80000000,

    IO_NOOP,
    IO_OK,
    IO_FAIL,

    IO_RESET,
    IO_PROG,
    
    ATOM_IO_MAX,
    ATOM_IO_CAP = 0x8FFFFFFF,
};

void atoms_register(void);
