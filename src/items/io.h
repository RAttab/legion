/* io.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

legion_packed enum io
{
    // We place the IO namespace right after the item namespace
    IO_MIN = 1 << 8,

    // Generic
    IO_NIL    = IO_MIN,
    IO_OK     = IO_MIN + 0x01,
    IO_FAIL   = IO_MIN + 0x02,
    IO_RETURN = IO_MIN + 0x03,
    IO_PING   = IO_MIN + 0x04,
    IO_PONG   = IO_MIN + 0x05,
    IO_STATUS = IO_MIN + 0x06,
    IO_STATE  = IO_MIN + 0x07,
    IO_RESET  = IO_MIN + 0x08,
    IO_ITEM   = IO_MIN + 0x09,
    IO_TAPE   = IO_MIN + 0x0A,
    IO_MOD    = IO_MIN + 0x0B,

    // Brain
    IO_ID         = IO_MIN + 0x10,
    IO_COORD      = IO_MIN + 0x11,
    IO_SEND       = IO_MIN + 0x12,
    IO_RECV       = IO_MIN + 0x13,
    IO_DBG_ATTACH = IO_MIN + 0x18,
    IO_DBG_DETACH = IO_MIN + 0x19,
    IO_DBG_BREAK  = IO_MIN + 0x1A,
    IO_DBG_STEP   = IO_MIN + 0x1B,

    // Lab
    IO_TAPE_AT    = IO_MIN + 0x20,
    IO_TAPE_KNOWN = IO_MIN + 0x21,
    IO_ITEM_BITS  = IO_MIN + 0x22,
    IO_ITEM_KNOWN = IO_MIN + 0x23,

    // Misc
    IO_GET      = IO_MIN + 0x80,
    IO_SET      = IO_MIN + 0x81,
    IO_SCAN     = IO_MIN + 0x82,
    IO_SCAN_VAL = IO_MIN + 0x83,
    IO_LAUNCH   = IO_MIN + 0x85,

    IO_MAX,
    IO_LEN = IO_MAX - IO_MIN,
};
