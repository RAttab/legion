/* op.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// op
// -----------------------------------------------------------------------------

enum { op_len = 8 };

enum legion_packed op_code
{
    OP_NOOP   = 0x00,

    OP_PUSH   = 0x10,
    OP_PUSHR  = 0x11,
    OP_PUSHF  = 0x12,
    OP_POP    = 0x13,
    OP_POPR   = 0x14,
    OP_DUPE   = 0x18,
    OP_SWAP   = 0x19,
    OP_ARG0   = 0x1A,
    OP_ARG1   = 0x1B,
    OP_ARG2   = 0x1C,
    OP_ARG3   = 0x1D,

    OP_NOT    = 0x20,
    OP_AND    = 0x21,
    OP_OR     = 0x22,
    OP_XOR    = 0x23,
    OP_BNOT   = 0x28,
    OP_BAND   = 0x29,
    OP_BOR    = 0x2A,
    OP_BXOR   = 0x2B,
    OP_BSL    = 0x2C,
    OP_BSR    = 0x2D,

    OP_NEG    = 0x30,
    OP_ADD    = 0x31,
    OP_SUB    = 0x32,
    OP_MUL    = 0x33,
    OP_LMUL   = 0x34,
    OP_DIV    = 0x35,
    OP_REM    = 0x36,

    OP_EQ     = 0x40,
    OP_NE     = 0x41,
    OP_GT     = 0x42,
    OP_GE     = 0x43,
    OP_LT     = 0x44,
    OP_LE     = 0x45,
    OP_CMP    = 0x46,

    OP_RET    = 0x50,
    OP_CALL   = 0x51,
    OP_LOAD   = 0x52,
    OP_JMP    = 0x58,
    OP_JZ     = 0x59,
    OP_JNZ    = 0x5A,

    OP_RESET  = 0x60,
    OP_YIELD  = 0x61,
    OP_TSC    = 0x62,
    OP_FAULT  = 0x63,

    OP_IO     = 0x68,
    OP_IOS    = 0x69,
    OP_IOR    = 0x6A,

    OP_PACK   = 0x80,
    OP_UNPACK = 0x81,

    OP_MAX_,
};

static_assert(sizeof(enum op_code) == 1);
