/* op.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// op
// -----------------------------------------------------------------------------

constexpr size_t vm_op_len = 8;

enum vm_op : uint8_t
{
    vm_op_noop   = 0x00,

    vm_op_push   = 0x10,
    vm_op_pushr  = 0x11,
    vm_op_pushf  = 0x12,
    vm_op_pop    = 0x13,
    vm_op_popr   = 0x14,
    vm_op_dupe   = 0x18,
    vm_op_swap   = 0x19,
    vm_op_arg0   = 0x1A,
    vm_op_arg1   = 0x1B,
    vm_op_arg2   = 0x1C,
    vm_op_arg3   = 0x1D,

    vm_op_not    = 0x20,
    vm_op_and    = 0x21,
    vm_op_or     = 0x22,
    vm_op_xor    = 0x23,
    vm_op_bnot   = 0x28,
    vm_op_band   = 0x29,
    vm_op_bor    = 0x2A,
    vm_op_bxor   = 0x2B,
    vm_op_bsl    = 0x2C,
    vm_op_bsr    = 0x2D,

    vm_op_neg    = 0x30,
    vm_op_add    = 0x31,
    vm_op_sub    = 0x32,
    vm_op_mul    = 0x33,
    vm_op_lmul   = 0x34,
    vm_op_div    = 0x35,
    vm_op_rem    = 0x36,

    vm_op_eq     = 0x40,
    vm_op_ne     = 0x41,
    vm_op_gt     = 0x42,
    vm_op_ge     = 0x43,
    vm_op_lt     = 0x44,
    vm_op_le     = 0x45,
    vm_op_cmp    = 0x46,

    vm_op_ret    = 0x50,
    vm_op_call   = 0x51,
    vm_op_load   = 0x52,
    vm_op_jmp    = 0x58,
    vm_op_jz     = 0x59,
    vm_op_jnz    = 0x5A,

    vm_op_reset  = 0x60,
    vm_op_yield  = 0x61,
    vm_op_tsc    = 0x62,
    vm_op_fault  = 0x63,

    vm_op_io     = 0x68,
    vm_op_ios    = 0x69,

    vm_op_pack   = 0x80,
    vm_op_unpack = 0x81,

    vm_op_max_,
};

static_assert(sizeof(enum vm_op) == 1);

enum vm_op_arg : uint8_t
{
    vm_op_arg_nil = 0,
    vm_op_arg_reg,
    vm_op_arg_lit,
    vm_op_arg_mod,
    vm_op_arg_off,
    vm_op_arg_len,
};

const char *vm_op_str(enum vm_op);
enum vm_op_arg vm_op_arg(enum vm_op);

size_t vm_op_arg_bytes(enum vm_op_arg);
size_t vm_op_arg_fmt(enum vm_op_arg, vm_word, char *dst, size_t len);
