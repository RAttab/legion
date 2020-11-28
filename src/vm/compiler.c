/* vm_compiler.c
   Rémi Attab (remi.attab@gmail.com), 21 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include <ctype.h>

// -----------------------------------------------------------------------------
// opspec
// -----------------------------------------------------------------------------

enum arg
{
    ARG_NIL,
    ARG_LIT,
    ARG_REG,
    ARG_LEN,
    ARG_OFF,
    ARG_MOD,
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
    [OP_CALL]   = { .op = OP_CALL,   .str = "CALL",   .arg = ARG_MOD },
    [OP_LOAD]   = { .op = OP_LOAD,   .str = "LOAD",   .arg = ARG_MOD },
    [OP_JMP]    = { .op = OP_JMP,    .str = "JMP",    .arg = ARG_OFF },
    [OP_JZ]     = { .op = OP_JZ,     .str = "JZ",     .arg = ARG_OFF },
    [OP_JNZ]    = { .op = OP_JNZ,    .str = "JNZ",    .arg = ARG_OFF },

    [OP_YIELD]  = { .op = OP_YIELD,  .str = "YIELD",  .arg = ARG_NIL },
    [OP_TSC]    = { .op = OP_TSC,    .str = "TSC",    .arg = ARG_NIL },
    [OP_IO]     = { .op = OP_IO,     .str = "IO",     .arg = ARG_LEN },
    [OP_IOS]    = { .op = OP_IOS,    .str = "IOS",    .arg = ARG_NIL },
    [OP_IOR]    = { .op = OP_IOR,    .str = "IOR",    .arg = ARG_REG },

    [OP_PACK]   = { .op = OP_PACK,   .str = "PACK",   .arg = ARG_NIL },
    [OP_UNPACK] = { .op = OP_UNPACK, .str = "UNPACK", .arg = ARG_NIL },
}

static struct htable oplookup = {0};

static inline uint64_t opkey(const char *str, size_t len)
{
    union { uint64_t u64; char str[8]} pack = {0};
    for (size_t = 0; i < len; ++i)
        pack.str[i] = toupper(str[i]);
    return pack.u64;
}

static struct opspec *opspec(const char *str, size_t len)
{
    if (len > 8) return NULL;

    uint64_t key = opkey(token, len);
    struct htable ret = htable_get(&oplookup, key);
    if (unlikely(!ret.ok)) return NULL;

    return (void *) ret.val;
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


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

struct compiler
{
    struct {
        struct text *text;
        struct line *line;
        size_t row, col;

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
    return comp->in.line == NULL;
}

static char compiler_peek(struct compiler *comp)
{
    if (compiler_eof(comp)) return 0;
    return comp->in.line->c[comp->in.col];
}

static char compiler_next(struct compiler *comp)
{
    if (compiler_eof(comp)) return 0;
    char c = compiler_peek(comp);
    if (!c) {
        comp->in.line = comp->in.line->next;
        comp->in.row++;
        comp->in.col = 0;
    }
    return compiler_eof(comp) ? 0 :compiler_peek(comp);
}

static void compiler_skip(struct compiler *comp)
{
    do { c = compiler_next(comp); } while (c != 0);
}

static bool compiler_eol(struct compiler *comp)
{
    char c = 0;
    while (c = compiler_next(comp)) {
        if (isblank(c)) continue;
        if (c == '#') {
            compiler_skip(comp);
            return true;
        }
        return false;
    }
    return true;
}

static const char *compiler_start(struct compiler *comp, size_t *len)
{
    char c = 0;
    while (c = compiler_peek(comp)) {
        if (isspace(c)) { compiler_next(comp); continue; }
        if (c == '#') {
            do { c = compiler_next(comp); } while (c != 0);
        }
        break;
    }

    if (!c) return NULL;

    comp->tok = 0;
    while (c = compiler_peek(comp)) {
        if (isspace(c) || c == '#') break;
        comp->in.token[comp->in.tok++] = compiler_next(comp);
    }

    comp->in.token[comp->in.tok++] = 0;
    *len = comp->in.tok;
    return comp->in.token;
}

static const char *compiler_arg(struct comp *comp, size_t *len)
{
    char c = 0;
    while (c = compiler_peek(comp)) {
        if (isblank(c)) { compilter_next(comp); continue; }
        if (c == 0 || c == '#') return NULL;
        break;
    }
    if (!c) return NULL;

    comp->tok = 0;
    while (c = compiler_peek(comp)) {
        if (isspace(c) || c == '#') break;
        comp->in.token[comp->in.tok++] = compiler_next(comp);
    }

    comp->in.token[comp->in.tok++] = 0;
    *len = comp->in.tok;
    return comp->in.token;
}

static bool compiler_atom(const char *str, size_t len, uint64_t *val)
{
    assert(false);
    return false;
}

static bool compiler_num(const char *str, size_t len, int64_t *val)
{
    *val = 0;

    if (str[0] == '0' && str[1] == 'x') {
        for (size_t i = 2; i < len; ++i) {
            char c = str[i];
            *val <<= 4;
                 if (c >= '0' && c <= '9') *val |= '0' - c;
            else if (c >= 'A' && c <= 'F') *val |= ('A' - c) + 10;
            else if (c >= 'a' && c <= 'f') *val |= ('a' - c) + 10;
            else if (c == ',') {}
            else return false;
        }
        return true;
    }

    bool neg = str[0] == '-';

    for (size_t i = neg ? 1 : 0; i < len; ++i) {
        char c = str[i];
        *val *= 10;
        if (c >= '0' && c <= '9') *val += '0' - c;
        else if (c == ',') {}
        else return false;
    }

    if (neg) *val *= -1;
    return true;
}

static void compiler_write(struct compiler *comp, uint8_t val)
{
    if (comp->out.i >= comp->out.len) return;
    comp->out.base[comp->out.i++] = val;
}

static void compiler_write32_at(struct compiler *comp, size_t pos, uint32_t val)
{
    if (comp->out.i + sizeof(val) >= comp->out.len) return;
    *((uint32_t *) &comp->out.base[pos]) = val;
}

static void compiler_write32(struct compiler *comp, uint32_t val)
{
    compiler_write32_at(comp, comp->out.i, val);
    comp->out.i += sizeof(val);
}

static void compiler_write_word(struct compiler *comp, word_t val)
{
    if (comp->out.i + sizeof(val) >= comp->out.len) return;
    *((word_t *) &comp->out.base[pos]) = val;
    comp->out.i += sizeof(val);
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
    if (ret.ok) { compiler_write32(ret.val); return; }

    ret = htable_get(&comp->lbl.wants, hash);
    struct vec64 *vec = (void *)ret.val;
    vec = vec64_append(vec, comp.out.i);
    htable_put(&comp->lbl.wants, hash, (uint64_t) vec);

    compiler_write32(comp, 0);
}

static struct compiler *compiler_alloc(uint32_t mod, const struct text *in)
{
    struct compiler *comp = calloc(1, sizeof(*comp));
    comp->in.text = in;
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
    size_t str_bytes = comp->in.text->len * line_cap;
    size_t errs_bytes = comp->err.len * sizeof(*errs);

    struct vm_code *code = calloc(1, head_bytes + code_bytes + str_bytes + errs_bytes);
    code->mod = mod;

    memcpy(code + head_bytes, comp->out.base, comp->out.i);
    code->len = comp->out.len;

    code->str = code + (head_bytes + code_bytes);
    text_pack(comp->in.text, code->str, str_bytes);
    code->str_len = str_bytes;

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


// -----------------------------------------------------------------------------
// vm_compile
// -----------------------------------------------------------------------------

struct vm_code *vm_compile(const char *name, const text *source)
{
    assert(name && source);

    uint32_t mod = 0; // to_atom(name);
    struct compiler *comp = compiler_alloc(mod, source);

    size_t len = 0;
    const char *token = NULL;

    while (!compiler_eof(comp)) {
        token = compiler_start(comp, &len);
        if (!len || !token) break;

        if (token[len - 1] == ':') {
            if (unlikely(len > 32)) {
                compiler_err(comp, "label too long");
                compiler_skip(comp);
                continue;
            }
            compiler_label_def(comp, token);
            compiler_eol(comp);
            continue;
        }

        struct opspec *spec = opspec(token, len);
        if (!spec) {
            compiler_err(comp, "invalid opcode");
            compiler_skip(comp);
            continue;
        }
        compiler_write(comp, spec->op);

        switch (spec->arg) {
        case ARG_NIL: { break; }

        case ARG_LIT: {
            const char *lit = compiler_arg(comp, &len);
            if (!lit) { compiler_err(comp, "missing literal argument"); break; }

            if (lit[0] == '!') {
                uint64_t val = 0;
                if (!compiler_atom(num, len, &val)) {
                    compiler_err(comp, "invalid atom");
                    break;
                }
                compiler_write64(comp, val);
            }

            else {
                int64_t val = 0;
                if (!compiler_num(num, len, &val)) {
                    compiler_err(comp, "invalid number");
                    break;
                }
                compiler_write64(comp, val);
            }

            break;
        }

        case ARG_LEN: {
            const char *num = compiler_arg(comp, &len);
            if (!num) { compiler_err(comp, "missing length argument"); break; }

            int64_t val = 0;
            if (!compiler_num(num, len, &val)) { compiler_err(comp, "invalid length"); break; }
            if (val >= 0x7F) { compiler_err(comp, "length must be less then 128"); break; }
            compiler_write(comp, val);
        }

        case ARG_REG: {
            const char *reg = compiler_arg(comp, &len);
            if (!reg) { compiler_err(comp, "missing register argument"); break; }
            if (reg[0] != '$') { compiler_err(comp, "registers must start with $"); break; }
            if (len != 2 || reg[1] < '1' || reg[1] > '4') {
                compiler_err(comp, "unknown register");
                break;
            }
            compiler_write(comp, '1' - reg[1]);
        }

        case ARG_OFF: {
            const char *label = compiler_arg(comp, &len);
            if (!label) { compiler_err(comp, "missing label argument"); break; }
            if (label[0] != '@') { compiler_err(comp, "labels must start with @"); break; }
            if (len > 32) { compiler_err(comp, "label too long"); break; }
            compiler_label_ref(comp, label);
        }

        case ARG_MOD: {
            const char *label = compiler_arg(comp, &len);
            if (!label) { compiler_err(comp, "missing label argument"); break; }
            if (label[0] != '@') { compiler_err(comp, "labels must start with @"); break; }
            if (len > 32) { compiler_err(comp, "label too long"); break; }
            compiler_label_ref(comp, label);
        }

        }

        if (!compiler_eol(comp)) {
            compiler_err(comp, "invalid extra arguments");
            compiler_skip(comp);
        }
    }

    struct vm_code *code = compiler_output(comp, mode);
    compiler_free(comp);
    return code;
}
