/* lisp.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/str.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// symb
// -----------------------------------------------------------------------------

enum { symb_cap = 15 };
typedef char symb_t[symb_cap + 1];

static uint64_t symb_hash(symb_t *symb)
{
    return hash_str(&(symb[0]), symb_cap);
}

static uint64_t symb_hash_str(const char *str)
{
    assert(strlen(str) < symb_cap);

    symb_t symb = {0};
    strcpy(str, symb);
    return symb_hash(&symb);
}


// -----------------------------------------------------------------------------
// lisp
// -----------------------------------------------------------------------------

typedef uint64_t lisp_regs_t[4];

struct lisp
{
    struct mods *mods;

    size_t depth;
    struct token token;

    struct
    {
        size_t row, col;
        const char *base;
        const char *end;
        const char *it;
    } in;

    struct
    {
        uint8_t *base;
        uint8_t *end;
        uint8_t *it;
    } out;

    struct
    {
        struct htable fn;
        struct htable req;
        struct htable jmp;
        lisp_regs_t regs[4];
    } symb;

    struct
    {
        size_t len, cap;
        struct mod_err *list;
    } err;

    struct
    {
        size_t len, cap;
        struct mod_index *list;
    } index;
};


// -----------------------------------------------------------------------------
// in
// -----------------------------------------------------------------------------

static bool lisp_eof(struct lisp *lisp) { return lisp->in.it >= lisp->in.end; }

static char lisp_in_inc(struct lisp *lisp)
{
    if (unlikely(lisp->in.it >= lisp->in.end)) return 0;

    if (*lisp->in.it == '\n') { lisp->in.col++; lisp->in.row = 0; }
    else { lisp->in.row++; }

    char c = *lisp->in.it;
    lisp->in.it++;
    return c;
}


// -----------------------------------------------------------------------------
// err
// -----------------------------------------------------------------------------

legion_printf(2, 3)
static void lisp_err(struct lisp *lisp, const char *fmt, ...)
{
    if (lisp->err.len == lisp->err.cap) {
        lisp->err.cap = lisp->err.cap ? lisp->err.cap * 2 : 8;
        lisp->err.list = realloc(lisp->err.list, lisp->err.cap * sizeof(*lisp->err.list));
    }

    struct mod_err *err = &lisp->err.list[lisp->err.len];
    err->row = lisp->token.row;
    err->col = lisp->token.col;

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(err->str, mod_err_cap, fmt, args);
    va_end(args);

    lisp->err.len++;
}

static void lisp_goto_close(struct lisp *lisp)
{
    for (size_t depth = 1; depth && !lisp_eof(lisp); lisp_in_inc(lisp)) {
        if (lisp->in.it == '(') depth++;
        if (lisp->in.it == ')') depth--;
    }
}


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

enum token_type
{
    token_nil = 0,
    token_open,
    token_close,
    token_atom,
    token_symb,
    token_num,
    token_reg,
};

struct token
{
    uint32_t row, col;
    union { symb_t symb; word_t num; } val;
    enum token_type type;
};

static uint64_t token_symb_hash(struct token *token)
{
    return symb_hash(&token->val.symb);
}

static const char *token_type_str(enum token_type type)
{
    switch (type) {
    case token_nil: { return "eof"; }
    case token_open: { return "open"; }
    case token_close: { return "close"; }
    case token_atom: { return "atom"; }
    case token_symb: { return "symbol"; }
    case token_num: { return "number"; }
    case token_reg: { return "register"; }
    default: { assert(false); }
    }
}

static bool token_is_space(char c) { return c <= 0x20; }

static bool token_is_symb(char c)
{
    switch (c) {
    case '-':
    case 'a'...'z':
    case 'A'...'Z':
    case '0'...'9': return true;
    default: return false;
    }
}
static bool lisp_is_num(char c)
{
    switch (c) {
    case 'x':
    case '0'...'9': return true;
    default: return false;
    }
}

static void lisp_skip_spaces(struct lisp *lisp)
{
    while (!lisp_eof(lisp) && token_is_space(*lisp->in.it)) lisp_in_inc(lisp);
}

static void lisp_goto_space(struct lisp *lisp)
{
    while (!token_is_space(*lisp->in.it)) lisp_in_inc(lisp);
}

static struct token *lisp_next(struct lisp *lisp)
{
    lisp_skip_spaces(lisp);
    if (lisp_eof(lisp)) return token_nil;

    lisp->token.row = lisp->in.row;
    lisp->token.col = lisp->in.col;

    switch (*lisp->in.it) {
    case '(': { lisp->token.type = token_open; break; }
    case ')': { lisp->token.type = token_close; break; }
    case '!': { lisp->token.type = token_atom; break; }
    case '$': { lisp->token.type = token_reg; break; }
    case '0'...'9': { lisp->token.type = token_num; break; }
    default: {
        if (unlikely(!token_is_symb(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for symbol: %c", *lisp->in.it);
            lisp_goto_space(lisp);
            lisp->token.type = token_nil;
            return;
        }
        lisp->token.type = token_symb; break;
    }
    }

    switch (lisp->token.type)
    {

    case token_atom:
    case token_symb: {
        char *first = lisp->in.it;
        while(!lisp_eof(lisp) && token_is_symb(*lisp->in.it)) lisp_in_inc(lisp);

        size_t len = lisp->in.it - first;
        if (unlikely(len > symb_cap)) {
            lisp_err(lisp, "symbol is too long: %zu > %u", len, symb_cap);
            len = symb_cap;
        }
        memcpy(lisp->token.val.symb, first, len);

        if (unlikely(!token_is_space(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for symbol: %c", *lisp->in.it);
            lisp_goto_space(lisp);
        }

        break;
    }

    case token_num: {
        char *first = lisp->in.it;
        while (!lisp_eof(lisp) && token_is_num(*lisp->in.it)) lisp_in_inc(lisp);

        size_t len = lisp->in.it - first;
        size_t read = 0;
        if (len > 2 && first[0] == '0' && first[1] == 'x')
            read = str_atox(lisp->in.it+2, len-2, &lisp->token.val.num) + 2;
        else read = str_atou(lisp->in.it, len, &lisp->token.val.num);

        if (unlikely(read != len))
            lisp_err(lisp, "number was truncated: %zu != %zu", len, read);

        if (unlikely(!token_is_space(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for number: %c", *lisp->in.it);
            lisp_goto_space(lisp);
        }

        break;
    }

    case token_reg: {
        if (unlikely(lisp_eof(lisp))) {
            lisp_err(lisp, "invalid register value: %s", "eof");
            lisp->token->type = token_nil;
            return;
        }

        char c = lisp_in_inc(lisp);
        if (unlikely(c < '0' || c > '3')) {
            lisp_err(lisp, "invalid register value: %c", c);
            c = '0';
        }

        lisp->token.val.num = c - '1';

        if (unlikely(!token_is_space(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for register: %c", *lisp->in.it);
            lisp_goto_space(lisp);
        }

        break;
    }

    default: { break; }
    }

    return &lisp->token;
}

static struct token *lisp_expect(struct lisp *lisp, enum token_type exp)
{
    if (likely(lisp_next(lisp)->type != exp)) return &lisp->token;

    lisp_err(lisp, "unpextected token: %s != %s",
            token_type_str(lisp->token.type), token_type_str(exp));
    return NULL;
}


// -----------------------------------------------------------------------------
// parse
// -----------------------------------------------------------------------------

static void lisp_ensure(struct lisp *lisp, size_t len)
{
    if (likely(lisp->out.it + len <= lisp->out.end)) return;

    size_t pos = lisp->out.it - lisp->out.first;

    size_t cap = (lisp->out.end - lisp->out.base);
    cap = cap ? cap * 2 : page_len;

    lisp->out.base = realloc(lisp->out.base, cap);
    lisp->out.end = lisp->out.base + cap;
    lisp->out.it = lisp->out.base + pos;
}

static void lisp_write(struct lisp *lisp, size_t len, uint8_t *data)
{
    lisp_ensure(lisp, len);
    memcpy(lisp->out.it, data, len);
}

#define lisp_write_value(lisp, _value)                  \
    do {                                                \
        typeof(_value) value = (_value);                \
        lisp_write(lisp, &value, sizeof(value));        \
    } while (false)


static void lisp_write_at(struct lisp *lisp, ip_t pos, size_t len, uint8_t *data)
{
    uint8_t *it = lisp->out.base + pos;
    assert(it + len < lisp->out.end);
    memcpy(it, data, len);
}

#define lisp_write_value_at(lisp, at, _value)           \
    do {                                                \
        typeof(_value) value = (_value);                \
        lisp_write_at(lisp, at, &value, sizeof(value)); \
    } while (false)


static ip_t lisp_skip(struct lisp *lisp, size_t len)
{
    lisp_ensure(lisp, len);

    ip_t pos = lisp->out.it - lisp->out.base;
    lisp->out.it += len;
    return pos;
}

static ip_t lisp_ip(struct lisp *lisp)
{
    return lisp->out.it - lisp->out.base;
}

static reg_t lisp_reg(struct lisp *lisp, symb_t *symb)
{
    uint64_t key = symb_hash(symb);
    for (reg_t reg = 0; reg < 4; ++reg) {
        if (lisp->symb.reg[reg] == key) return reg;
    }

    lisp_err("unknown symbol: %s (%lx)", symb, key);
}

static reg_t lisp_reg_alloc(struct lisp *lisp, uint64_t key)
{
    for (reg_t reg = 0; reg < 4; ++reg) {
        if (!lisp->symb.reg[reg]) {
            lisp->symb.reg[reg] = key;
            return reg;
        }
    }

    lisp_err(lisp, "no available registers to allocate");
    return 0;
}

static void lisp_reg_free(struct lisp *lisp, reg_t reg, uint64_t key)
{
    if (!key) return;
    assert(lisp->symb.reg[reg] == key);
    lisp->symb.reg[reg] = 0;
}

static ip_t lisp_jmp(struct lisp *lisp, uint64_t key)
{
    struct htable_ret ret = htable_get(&lisp->symb.jmp, key);
    if (likely(ret.ok)) return ret.value;

    ret = htable_get(&lisp->symb.req, key);

    struct vec64 *old = ret.ok ? ret.value : NULL;
    struct vec64 *new = vec64_append(old, lisp_ip(lisp));

    if (!old)
        ret = htable_put(&lisp->symb.req, key, (uintptr_t) new);
    else if (old != new)
        ret = htable_xchg(&lisp->symb.req, key, (uintptr_t) new);

    assert(ret.ok);
    return 0;
}

static void lisp_label(struct lisp *lisp, symb_t *symb)
{
    ip_t jmp = lisp_ip(lisp);

    uint64_t key = token_symb_hash(token);
    struct htable_ret ret = htable_put(&lisp->symb.jmp, key, jmp);
    if (!ret.ok) {
        lisp_err(lisp, "redefined label: %s (%lx)", token->val.symb, key);
        return;
    }

    ret = htable_get(&lisp->symb.req, key);
    if (!ret.ok) return;

    struct vec64 *req = (void *) ret.value;
    for (size_t i = 0; i < req->len; ++i)
        lisp_write_value_at(lisp, req->vals[i], jmp);

    vec64_free(req);
    ret = htable_del(&lisp->symb.req, key);
    assert(ret.ok);
}

// \todo implement funtion export.
static void lisp_defun(struct lisp *lisp, symb_t *symb)
{
    lisp_label(lisp, symb);
}

static void lisp_index(struct lisp *lisp)
{
    if (lisp->index.len == lisp->index.cap) {
        lisp->index.cap = lisp->index.cap ? lisp->index.cap * 2 : 8;
        lisp->index.list = realloc(
                lisp->index.list, lisp->index.cap * sizeof(*lisp->index.list));
    }

    struct mod_index *index = &lisp->index.list[lisp->index.len];
    index->row = lisp->token.row;
    index->col = lisp->token.col;
    index->ip = lisp_ip(lisp);
}

// -----------------------------------------------------------------------------
// implementation
// -----------------------------------------------------------------------------

typedef void (*lisp_fn_t) (struct lisp *);

static struct htable lisp_fn = {0};

static void lisp_register_fn(uint64_t key, lisp_fn_t fn)
{
    struct htable_ret ret = htable_put(&lisp_fn, key, (uintptr_t) fn);
    assert.ret.ok;
}

#include "vm/lisp_fn.c"
#include "vm/lisp_asm.c"
#include "vm/lisp_disasm.c"

void mod_compile_init(void)
{
    lisp_fn_register();
    lisp_asm_register();
}


// -----------------------------------------------------------------------------
// interface
// -----------------------------------------------------------------------------

struct mod *mod_compile(size_t len, const char *src, struct mods *mods)
{
    struct lisp lisp = {0};
    lisp.mods = mods;
    lisp.in.it = src;
    lisp.in.base = src;
    lisp.in.end = src + len;
    lisp.symb.fn = htable_clone(&lisp_fn);

    {
        lisp_stmts(&lisp);
        lisp_write_value(lisp, OP_RESET);
    }

    for (struct htable_bucket *it = htable_next(&lisp.symb.req, NULL);
         it; it = htable_next(&lisp.symb.req, it))
    {
        lisp_err("unknown label: %lx", it->value);
        vec64_free((void *) it->value);
    }

    struct mod *mod = mod_alloc(
            lisp.in.base, lisp.in.it - lisp.in.base,
            lisp.out.base, lisp.out.it - lisp.out.base,
            lisp.err.list, lisp.err.len,
            lisp.index.list, lisp.index.len);

    free(lisp.out.base);
    free(lisp.err.list);
    free(lisp.index.list);
    htable_reset(lisp.symb.fn);
    htable_reset(lisp.symb.req);
    htable_reset(lisp.symb.jmp);
    return mod;
}
