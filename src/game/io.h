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

    IO_STATUS,
    IO_STATE,

    IO_COORD,

    IO_RESET,
    IO_ITEM,
    IO_TAPE,
    IO_MOD,

    IO_GET,
    IO_SET,
    IO_VAL,

    IO_SEND,
    IO_RECV,

    IO_SCAN,
    IO_SCAN_VAL,

    IO_LAUNCH,

    IO_DBG_ATTACH,
    IO_DBG_DETACH,
    IO_DBG_BREAK,
    IO_DBG_STEP,

    IO_LEARN,
    IO_TAPE_DATA,
    IO_TAPE_AT,

    ATOM_IO_MAX,
};

