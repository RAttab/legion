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


struct compiler
{
    struct {
        const char *base;
        size_t i, len, line;

        size_t tok;
        char token[128];
    } in;

    struct {
        uint8_t *base;
        size_t i, len;
    } out;

    struct {
        size_t len, cap;
        struct vm_errors *list;
    } err;

    struct {
        struct htable is;
        struct htable wants;
    } lbl;
};

static bool compiler_eof(struct compiler *comp)
{
    return comp->in.i >= comp->in.len;
}

static char compiler_peek(struct compiler *comp)
{
    if (compiler_eof(comp)) return 0;
    return comp->in.base[comp->i];
}

static char compiler_next(struct compiler *comp)
{
    if (compiler_eof(comp)) return 0;
    if (comp->in.base[comp->i] == '\n') comp->in.line++;
    return comp->in.base[comp->i++];
}

static void compiler_skip(struct compiler *comp)
{
    for (char c = compiler_next(comp); !c || c == '\n'; c = compiler_next(comp));
}

static const char *compiler_token(struct compiler *comp, size_t *len)
{
    comp->in.tok = 0;
    comp->in.token[0] = 0;

    while (true) {
        while (isspace(compiler_peek(comp))) compiler_next(comp);
        if (compiler_peek(comp) != '#') break;
        compiler_skip(comp);
    }

    while (true) {
        char c = compiler_next(comp);
        if (!c || c == '#' || isspace(c)) {
            comp->in.token[comp->in.tok++] = 0;
            *len = comp->in.tok - 1;
            return comp->in.token;
        }
        comp->in.token[comp->in.tok++] = c;
    }
}

static void compiler_write(struct compiler *comp, uint8_t val)
{
    if (comp->out.i >= comp->out.len) return;
    comp->out.base[comp->out.i++] = val;
}

static void compiler_write64_at(struct compiler *comp, size_t pos, uint8_t val)
{
    if (comp->out.i + 8 >= comp->out.len) return;
    *((uint64_t *) &comp->out.base[pos]) = val;
}

static void compiler_write64(struct compiler *comp, uint64_t val)
{
    compiler_write64_at(comp, comp->out.i, val);
    comp->out.i += 8;
}

static void compiler_err(struct compiler *comp, const char *fmt, ...)
    legion_printf(2, 3)
{
    if (comp->err.len == comp->err.cap) {
        size_t len = comp->err.cap ? comp->err.cap * 2 : 2;
        comp->err.list = realloc(comp->err.list, len * sizeof(*comp->err.list));
    }

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(comp->err.list[comp->err.len], vm_errors_cap, fmt, args);
    va_end(args);

    comp->err.len++;
}

static void compiler_label_def(struct compiler *comp, const char *label)
{
    uint64_t hash = hash_str(label);
    struct htable_ret ret = htable_put(&comp->lbl.is, hash, comp->out.i);
    if (!ret.ok) { compiler_err(comp, "refined label: %s", label); return; }

    ret = htable_get(&comp->lbl.wants, hash);
    if (!ret.ok) return;

    struct vec64 *vec = (void *) ret.val;
    for (size_t i = 0; i < vec->len; ++i)
        compiler_write64_at(comp, vec->vals[i], comp->out.i);

    (void) htable_del(&comp->lbl.wants, hash);
}

static void compiler_label_ref(struct compiler *comp, const char *label)
{
    uint64_t hash = hash_str(label);

    struct htable_ret ret = htable_get(&comp->lbl.is, hash);
    if (ret.ok) { compiler_write64(ret.val); return; }

    ret = htable_get(&comp->lbl.wants, hash);
    struct vec64 *vec = (void *)ret.val;
    vec = vec64_append(vec, comp.out.i);
    htable_put(&comp->lbl.wants, hash, (uint64_t) vec);

    compiler_write64(comp, 0);
}

static struct compiler *compiler_alloc(const char *in, size_t len)
{
    struct compiler *comp = calloc(1, sizeof(*comp));
    comp->in.base = in;
    comp->in.len = len;

    comp->out.len = 1 << 16;
    comp->out.base = calloc(1, comp->out.len);

    return comp;
}

static struct vm_code *compiler_output(struct compiler *comp, uint64_t mod)
{
    if (comp->out.i == comp->out.len)
        compiler_err(comp, "program too big");

    struct htable_bucket *bucket = htable_next(&comp->lvl.want, NULL);
    for (; bucket; bucket = htable_next(&comp->lvl.want, bucket))
        compiler_err(comp, "missing label at '%lu'", bucket->val);

    size_t head_bytes = sizeof(struct vm_code);
    size_t code_bytes = comp->out.len;
    size_t str_bytes = comp->in.len + 1;
    size_t errs_bytes = comp->err.len * sizeof(*errs);

    struct vm_code *code = calloc(1, head_bytes + code_bytes + str_bytes + errs_bytes);
    code->mod = mod;

    memcpy(code + head_bytes, comp->out.base, comp->out.i);
    code->len = comp->out.len;

    code->str = code + (head_bytes + code_bytes);
    memcpy(code->str, comp->in.str, len);
    code->str_len = comp->in.len;
    code->str[len] = 0;

    code->errs = code + (head_bytes + code_bytes + str_bytes);
    memcpy(code->errs, errs, errs_bytes);
    code->errs_len = errs_len;

    return code;
}

static void compiler_free(struct compilre *comp)
{
    free(comp->err.list);
    free(comp->err.list);
    htable_reset(&comp->lbl.is);
    htable_reset(&comp->lbl.wants);
    free(comp);
}

struct vm_code *vm_compile(uint32_t mod, const char *str, size_t len)
{
    assert(str && len);

    struct compiler *comp = compiler_alloc(str, len);

    while (!compiler_eof(comp)) {
        size_t len = 0;
        const char *token = compiler_token(comp, &len);
        if (!len) return;
        
        

    }

    struct vm_code *code = compiler_output(comp, mode);
    compiler_free(comp);
    return code;
}
