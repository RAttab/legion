/* op.c
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "op.h"
#include "utils/htable.h"

#include <ctype.h>


// -----------------------------------------------------------------------------
// op
// -----------------------------------------------------------------------------

const struct op_spec op_specs[] = {
    [OP_NOOP]   = { .op = OP_NOOP,   .str = "NOOP",   .arg = ARG_NIL },

    [OP_PUSH]   = { .op = OP_PUSH,   .str = "PUSH",   .arg = ARG_LIT },
    [OP_PUSHR]  = { .op = OP_PUSHR,  .str = "PUSHR",  .arg = ARG_REG },
    [OP_PUSHF]  = { .op = OP_PUSHF,  .str = "PUSHF",  .arg = ARG_NIL },
    [OP_POP]    = { .op = OP_POP,    .str = "POP",    .arg = ARG_NIL },
    [OP_POPR]   = { .op = OP_POPR,   .str = "POPR",   .arg = ARG_REG },
    [OP_DUPE]   = { .op = OP_DUPE,   .str = "DUPE",   .arg = ARG_NIL },
    [OP_SWAP]   = { .op = OP_SWAP,   .str = "SWAP",   .arg = ARG_NIL },

    [OP_NOT]    = { .op = OP_NOT,    .str = "NOT",    .arg = ARG_NIL },
    [OP_AND]    = { .op = OP_AND,    .str = "AND",    .arg = ARG_NIL },
    [OP_OR]     = { .op = OP_OR,     .str = "OR",     .arg = ARG_NIL },
    [OP_XOR]    = { .op = OP_XOR,    .str = "XOR",    .arg = ARG_NIL },
    [OP_BNOT]   = { .op = OP_BNOT,   .str = "BNOT",   .arg = ARG_NIL },
    [OP_BAND]   = { .op = OP_BAND,   .str = "BAND",   .arg = ARG_NIL },
    [OP_BOR]    = { .op = OP_BOR,    .str = "BOR",    .arg = ARG_NIL },
    [OP_BXOR]   = { .op = OP_BXOR,   .str = "BXOR",   .arg = ARG_NIL },
    [OP_BSL]    = { .op = OP_BSL,    .str = "BSL",    .arg = ARG_NIL },
    [OP_BSR]    = { .op = OP_BSR,    .str = "BSR",    .arg = ARG_NIL },

    [OP_NEG]    = { .op = OP_NEG,    .str = "NEG",    .arg = ARG_NIL },
    [OP_ADD]    = { .op = OP_ADD,    .str = "ADD",    .arg = ARG_NIL },
    [OP_SUB]    = { .op = OP_SUB,    .str = "SUB",    .arg = ARG_NIL },
    [OP_MUL]    = { .op = OP_MUL,    .str = "MUL",    .arg = ARG_NIL },
    [OP_LMUL]   = { .op = OP_LMUL,   .str = "LMUL",   .arg = ARG_NIL },
    [OP_DIV]    = { .op = OP_DIV,    .str = "DIV",    .arg = ARG_NIL },
    [OP_REM]    = { .op = OP_REM,    .str = "REM",    .arg = ARG_NIL },

    [OP_EQ]     = { .op = OP_EQ,     .str = "EQ",     .arg = ARG_NIL },
    [OP_NE]     = { .op = OP_NE,     .str = "NE",     .arg = ARG_NIL },
    [OP_GT]     = { .op = OP_GT,     .str = "GT",     .arg = ARG_NIL },
    [OP_LT]     = { .op = OP_LT,     .str = "LT",     .arg = ARG_NIL },
    [OP_CMP]    = { .op = OP_CMP,    .str = "CMP",    .arg = ARG_NIL },

    [OP_RET]    = { .op = OP_RET,    .str = "RET",    .arg = ARG_NIL },
    [OP_CALL]   = { .op = OP_CALL,   .str = "CALL",   .arg = ARG_MOD },
    [OP_LOAD]   = { .op = OP_LOAD,   .str = "LOAD",   .arg = ARG_MOD },
    [OP_JMP]    = { .op = OP_JMP,    .str = "JMP",    .arg = ARG_OFF },
    [OP_JZ]     = { .op = OP_JZ,     .str = "JZ",     .arg = ARG_OFF },
    [OP_JNZ]    = { .op = OP_JNZ,    .str = "JNZ",    .arg = ARG_OFF },

    [OP_RESET]  = { .op = OP_RESET,  .str = "RESET",  .arg = ARG_NIL },
    [OP_YIELD]  = { .op = OP_YIELD,  .str = "YIELD",  .arg = ARG_NIL },
    [OP_TSC]    = { .op = OP_TSC,    .str = "TSC",    .arg = ARG_NIL },

    [OP_IO]     = { .op = OP_IO,     .str = "IO",     .arg = ARG_LEN },
    [OP_IOS]    = { .op = OP_IOS,    .str = "IOS",    .arg = ARG_NIL },
    [OP_IOR]    = { .op = OP_IOR,    .str = "IOR",    .arg = ARG_REG },

    [OP_PACK]   = { .op = OP_PACK,   .str = "PACK",   .arg = ARG_NIL },
    [OP_UNPACK] = { .op = OP_UNPACK, .str = "UNPACK", .arg = ARG_NIL },
};

static struct htable op_lookup = {0};

static uint64_t op_key(const char *str, size_t len)
{
    union { uint64_t u64; char str[op_len]; } pack = {0};
    for (size_t i = 0; i < len; ++i)
        pack.str[i] = toupper(str[i]);
    return pack.u64;
}

struct op_spec *op_spec(const char *str, size_t len)
{
    if (len > 8) return NULL;

    uint64_t key = op_key(str, len);
    struct htable_ret ret = htable_get(&op_lookup, key);
    if (unlikely(!ret.ok)) return NULL;

    return (void *) ret.value;
}

void vm_compile_init(void)
{
    htable_reserve(&op_lookup, OP_MAX_);

    for (size_t i = 0; i < OP_MAX_; ++i) {
        const struct op_spec *spec = &op_specs[i];
        if (i && !spec->op) continue;

        uint64_t key = op_key(spec->str, op_len);
        uint64_t val = (uint64_t) spec;
        struct htable_ret ret = htable_put(&op_lookup, key, val);
        assert(ret.ok && !ret.value);
    }
}
