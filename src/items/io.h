/* io.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

enum legion_packed io
{
    // We place the IO namespace right after the item namespace
    IO_MIN = 1 << 8,

    // Generic
    IO_NIL    = IO_MIN,
    IO_OK     = IO_MIN + 0x01,
    IO_FAIL   = IO_MIN + 0x02,
    IO_STEP   = IO_MIN + 0x03,
    IO_ARRIVE = IO_MIN + 0x04,
    IO_RETURN = IO_MIN + 0x05,
    IO_PING   = IO_MIN + 0x06,
    IO_PONG   = IO_MIN + 0x07,
    IO_STATE  = IO_MIN + 0x08,
    IO_RESET  = IO_MIN + 0x09,
    IO_ITEM   = IO_MIN + 0x0A,
    IO_TAPE   = IO_MIN + 0x0B,
    IO_MOD    = IO_MIN + 0x0C,
    IO_LOOP   = IO_MIN + 0x0D,
    IO_VALUE  = IO_MIN + 0x0E,

    // Brain
    IO_ID         = IO_MIN + 0x10,
    IO_LOG        = IO_MIN + 0x11,
    IO_TICK       = IO_MIN + 0x12,
    IO_COORD      = IO_MIN + 0x13,
    IO_NAME       = IO_MIN + 0x14,
    IO_SEND       = IO_MIN + 0x18,
    IO_RECV       = IO_MIN + 0x19,
    IO_DBG_ATTACH = IO_MIN + 0x1A,
    IO_DBG_DETACH = IO_MIN + 0x1B,
    IO_DBG_BREAK  = IO_MIN + 0x1C,
    IO_DBG_STEP   = IO_MIN + 0x1D,
    IO_SPECS      = IO_MIN + 0x1F,

    // Lab
    IO_TAPE_AT    = IO_MIN + 0x20,
    IO_TAPE_KNOWN = IO_MIN + 0x21,
    IO_ITEM_BITS  = IO_MIN + 0x22,
    IO_ITEM_KNOWN = IO_MIN + 0x23,

    // TX/RX
    IO_CHANNEL  = IO_MIN + 0x30,
    IO_TRANSMIT = IO_MIN + 0x31,
    IO_RECEIVE  = IO_MIN + 0x32,
    // Nomad
    IO_PACK     = IO_MIN + 0x38,
    IO_LOAD     = IO_MIN + 0x39,
    IO_UNLOAD   = IO_MIN + 0x3A,

    // Misc
    IO_GET      = IO_MIN + 0x80,
    IO_SET      = IO_MIN + 0x81,
    IO_CAS      = IO_MIN + 0x82,
    IO_SCAN     = IO_MIN + 0x83,
    IO_PROBE    = IO_MIN + 0x84,
    IO_LAUNCH   = IO_MIN + 0x85,
    IO_TARGET   = IO_MIN + 0x86,
    IO_GROW     = IO_MIN + 0x87,
    IO_INPUT    = IO_MIN + 0x88,
    IO_ACTIVATE = IO_MIN + 0x89,

    // State
    IO_HAS_ITEM = IO_MIN + 0xF0,
    IO_HAS_LOOP = IO_MIN + 0xF1,
    IO_SIZE     = IO_MIN + 0xF2,
    IO_RATE     = IO_MIN + 0xF3,
    IO_WORK     = IO_MIN + 0xF4,
    IO_OUTPUT   = IO_MIN + 0xF5,
    IO_CARGO    = IO_MIN + 0xF6,

    IO_MAX,
    IO_LEN = IO_MAX - IO_MIN,
};

static_assert(sizeof(enum io) == 2);


// -----------------------------------------------------------------------------
// ioerr
// -----------------------------------------------------------------------------

enum legion_packed ioe
{
    IOE_NIL = 0,
    IOE_MIN = 2 << 8,

    IOE_MISSING_ARG   = IOE_MIN + 0x00,
    IOE_INVALID_STATE = IOE_MIN + 0x01,
    IOE_VM_FAULT      = IOE_MIN + 0x02,
    IOE_STARVED       = IOE_MIN + 0x03,
    IOE_OUT_OF_RANGE  = IOE_MIN + 0x04,
    IOE_OUT_OF_SPACE  = IOE_MIN + 0x05,
    IOE_INVALID_SPEC  = IOE_MIN + 0x06,

    IOE_A0_INVALID = IOE_MIN + 0x10,
    IOE_A0_UNKNOWN = IOE_MIN + 0x11,
    IOE_A1_INVALID = IOE_MIN + 0x20,
    IOE_A1_UNKNOWN = IOE_MIN + 0x21,

    IOE_MAX,
    IOE_LEN = IOE_MAX - IOE_MIN,
};

static_assert(sizeof(enum ioe) == 2);
