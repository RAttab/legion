/* vm_compiler.c
   Rémi Attab (remi.attab@gmail.com), 21 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include <ctype.h>

enum arg
{
    ARG_NIL,
    ARG_LIT,
    ARG_REG,
    ARG_NUM,
    ARG_ADDR,
}

struct opspec
{
    enum opcodes op;
    const char str[8];
    enum arg arg;
}

static const struct opspec[] = {
    [OP_NOOP]   = { .op = OP_NOOP,   .str = "NOOP",   .arg = ARG_NIL },

    [OP_PUSH]   = { .op = OP_PUSH,   .str = "PUSH",   .arg = ARG_LIT },
    [OP_PUSHR]  = { .op = OP_PUSHR,  .str = "PUSHR",  .arg = ARG_REG },
    [OP_PUSHF]  = { .op = OP_PUSHF,  .str = "PUSHF",  .arg = ARG_NIL },
    [OP_POP]    = { .op = OP_POP,    .str = "POP",    .arg = ARG_NIL },
    [OP_POPR]   = { .op = OP_POPR,   .str = "POPR",   .arg = ARG_REG },
    [OP_DUPE]   = { .op = OP_DUPE,   .str = "DUPE",   .arg = ARG_NIL },
    [OP_FLIP]   = { .op = OP_FLIP,   .str = "FLIP",   .arg = ARG_NIL },

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
    [OP_CALL]   = { .op = OP_CALL,   .str = "CALL",   .arg = ARG_ADDR },
    [OP_LOAD]   = { .op = OP_LOAD,   .str = "LOAD",   .arg = ARG_NIL },
    [OP_JMP]    = { .op = OP_JMP,    .str = "JMP",    .arg = ARG_ADDR },
    [OP_JZ]     = { .op = OP_JZ,     .str = "JZ",     .arg = ARG_ADDR },
    [OP_JNZ]    = { .op = OP_JNZ,    .str = "JNZ",    .arg = ARG_ADDR },

    [OP_YIELD]  = { .op = OP_YIELD,  .str = "YIELD",  .arg = ARG_NIL },
    [OP_READ]   = { .op = OP_READ,   .str = "READ",   .arg = ARG_NIL },
    [OP_WRITE]  = { .op = OP_WRITE,  .str = "WRITE",  .arg = ARG_LEN },

    [OP_PACK]   = { .op = OP_PACK,   .str = "PACK",   .arg = ARG_NIL },
    [OP_UNPACK] = { .op = OP_UNPACK, .str = "UNPACK", .arg = ARG_NIL },
}

static struct htable oplookup = {0};

static inline uint64_t opkey(char str[8])
{
    return (union { uint64_t u64; char str[8]}) { .str = str }.u64;
}

void vm_compile_init()
{
    const size_t opmax = 48;
    htable_reserve(&oplookup, opmax);

    for (size_t i = 0; i < opmax; ++i) {
        if (!opspec[i].op) return;

        uint64_t key = opkey(opspec[i].str);
        uint64_t val = (uint64_t) (&opspec[i]);
        struct htable_ret ret = htable_put(&oplookup, key, val);
        assert(ret.ok && !ret.val);
    }
}

struct ctx
{
    const char *in;
    size_t i, len;

    size_t line, col;
};

static char ctx_peek(struct ctx *ctx)
{
    if (ctx->i == ctx->len) return 0;
    return ctx->in[i];
}

static bool ctx_inc(struct ctx *ctx)
{
    if (ctx->i == ctx->len) return false;

    if (ctx->in[ctx->i] != '\n') { ++ctx->col; }
    else { ctx->line++; ctx->col = 0; }

    ctx->i++;
    return ctx->i < ctx->len;
}

static size_t ctx_token(struct ctx *ctx, char *tok, size_t len)
{
    while (isspace(ctx_peek(ctx)) && ctx_inc(ctx));

    size_t j = 0;
    do {
        char c = ctx_peek(ctx);
        if (c && !isspace(c)) tok[j] = tolower(c);
    } while (++j < len && ctx_inc(ctx));

    return j;
}

static void ctx_skip_line(struct ctx *ctx)
{
    size_t line = ctx->line;
    while (line == ctx->line) ctx_inc(ctx);
}

struct vm_code *vm_compile(uint32_t key, const char *str, size_t len)
{
    assert(str && len);
    
    enum { max_code_len = 1 << 16 };
    struct vm_code *code = calloc(1, sizeof(*code) + max_code_len);
    code->mod = key;

    bool err = false;
    struct ctx ctx = { .in = str, .len = len };

    while (true) {
        char tok_op[9] = {0};
        i += ctx_token(&ctx, tok_op, 8);
        
        struct htable_ret ret = htable_get(&oplookup, opkey(tok_op));
        if (!ret.ok) {
            fprintf(stderr, "%zu:%zu: invalid opcode: %s\n", ctx->line, ctx->col, tok_op);
            ctx_skip(ctx);
            continue;
        }

        const struct opspec *spec = (const struct opspec *) ret.val;
        code->prog[
    }
    
    
    return NULL;
}
