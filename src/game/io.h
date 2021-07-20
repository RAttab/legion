/* io.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// atoms_io
// -----------------------------------------------------------------------------

legion_packed enum atom_io
{
    // 31 is the sign bit and would cause sign-bit extension when converting to
    // u64. Need something less dumb then this.
    ATOM_IO_MIN = 1 << 30,

    IO_NIL = ATOM_IO_MIN,
    IO_OK,
    IO_FAIL,

    IO_PING,
    IO_PONG,

    IO_RESET,
    IO_ITEM,
    IO_PROG,
    IO_MOD,

    IO_GET,
    IO_SET,
    IO_VAL,

    IO_SEND,
    IO_RECV,

    IO_SCAN,
    IO_RESULT,

    IO_LAUNCH,

    ATOM_IO_MAX,
};
