/* atoms.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// atoms_io
// -----------------------------------------------------------------------------

enum legion_packed atom_io
{
    ATOM_IO_MIN = 1 << 31,

    IO_NIL = ATOM_IO_MIN,

    IO_PING,
    IO_PONG,

    IO_RESET,
    IO_PROG,

    IO_GET,
    IO_SET,
    IO_VAL,

    IO_SEND,
    IO_RECV,

    ATOM_IO_MAX,
};

void atoms_register(void);
