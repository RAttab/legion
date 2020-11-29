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

    // Common
    IO_ID,
    IO_TARGET,
    IO_SEND,
    IO_SENDN,
    IO_RECV,
    IO_RECVN,
    IO_CARGO,
    IO_VENT,

    // Worker
    IO_DOCK,
    IO_UNDOCK,
    IO_TAKE,
    IO_PUT,
    IO_HARVEST,
    
    ATOM_IO_MAX,
    ATOM_IO_CAP = 0x8FFFFFFF,
};
