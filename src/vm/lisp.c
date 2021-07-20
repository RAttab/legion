/* lisp.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/mod.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/htable.h"


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
    uint32_t row, col, len;
    union { struct symbol symb; word_t num; } val;
    enum token_type type;
};


static uint64_t token_symb_hash(struct token *token)
{
    return symbol_hash(&token->val.symb);
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


// -----------------------------------------------------------------------------
// lisp
// -----------------------------------------------------------------------------

struct lisp
{
    struct mods *mods;
    struct atoms *atoms;

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
        uint64_t regs[4];
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

    if (*lisp->in.it == '\n') { lisp->in.row++; lisp->in.col = 0; }
    else { lisp->in.col++; }

    lisp->in.it++;
    return *lisp->in.it;
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
    err->len = lisp->token.len;

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(err->str, mod_err_cap, fmt, args);
    va_end(args);

    lisp->err.len++;
}

static void lisp_goto_close(struct lisp *lisp)
{
    for (size_t depth = 1; depth && !lisp_eof(lisp); lisp_in_inc(lisp)) {
        if (*lisp->in.it == '(') depth++;
        if (*lisp->in.it == ')') depth--;
    }
}


// -----------------------------------------------------------------------------
// out
// -----------------------------------------------------------------------------

static void lisp_ensure(struct lisp *lisp, size_t len)
{
    if (likely(lisp->out.it + len <= lisp->out.end)) return;

    size_t pos = lisp->out.it - lisp->out.base;
    size_t cap = (lisp->out.end - lisp->out.base);
    cap = cap ? cap * 2 : page_len;

    lisp->out.base = realloc(lisp->out.base, cap);
    lisp->out.end = lisp->out.base + cap;
    lisp->out.it = lisp->out.base + pos;
}

static void lisp_write(struct lisp *lisp, size_t len, const void *data)
{
    lisp_ensure(lisp, len);
    memcpy(lisp->out.it, data, len);
    lisp->out.it += len;
}

// `sizeof(enum op_code) != sizeof(OP_XXX)` so gotta do it manually.
static void lisp_write_op(struct lisp *lisp, enum op_code op)
{
    lisp_write(lisp, sizeof(op), &op);
}

#define lisp_write_value(lisp, _value)                  \
    do {                                                \
        typeof(_value) value = (_value);                \
        lisp_write(lisp, sizeof(value), &value);        \
    } while (false)


static void lisp_write_at(
        struct lisp *lisp, ip_t pos, size_t len, const void *data)
{
    uint8_t *it = lisp->out.base + pos;
    assert(it + len < lisp->out.end);
    memcpy(it, data, len);
}

#define lisp_write_value_at(lisp, at, _value)           \
    do {                                                \
        typeof(_value) value = (_value);                \
        lisp_write_at(lisp, at, sizeof(value), &value); \
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


// -----------------------------------------------------------------------------
// index
// -----------------------------------------------------------------------------

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
// regs
// -----------------------------------------------------------------------------

static reg_t lisp_reg(struct lisp *lisp, const struct symbol *symbol)
{
    uint64_t key = symbol_hash(symbol);
    for (reg_t reg = 0; reg < 4; ++reg) {
        if (lisp->symb.regs[reg] == key) return reg;
    }

    lisp_err(lisp, "unknown symbol: %s (%lx)", symbol->c, key);
    return 0;
}

static reg_t lisp_reg_alloc(struct lisp *lisp, uint64_t key)
{
    for (reg_t reg = 0; reg < 4; ++reg) {
        if (!lisp->symb.regs[reg]) {
            lisp->symb.regs[reg] = key;
            return reg;
        }
    }

    lisp_err(lisp, "no available registers to allocate");
    return 0;
}

static void lisp_reg_free(struct lisp *lisp, reg_t reg, uint64_t key)
{
    if (!key) return;
    assert(lisp->symb.regs[reg] == key);
    lisp->symb.regs[reg] = 0;
}


// -----------------------------------------------------------------------------
// jmp
// -----------------------------------------------------------------------------

static ip_t lisp_jmp(struct lisp *lisp, uint64_t key)
{
    struct htable_ret ret = htable_get(&lisp->symb.jmp, key);
    if (likely(ret.ok)) return ret.value;

    ret = htable_get(&lisp->symb.req, key);
    struct vec64 *old = ret.ok ? (void *) ret.value : NULL;
    struct vec64 *new = vec64_append(old, lisp_ip(lisp));

    if (!old)
        ret = htable_put(&lisp->symb.req, key, (uintptr_t) new);
    else if (old != new)
        ret = htable_xchg(&lisp->symb.req, key, (uintptr_t) new);

    assert(ret.ok);
    return 0;
}

static void lisp_label(struct lisp *lisp, const struct symbol *symbol)
{
    ip_t jmp = lisp_ip(lisp);

    uint64_t key = symbol_hash(symbol);
    struct htable_ret ret = htable_put(&lisp->symb.jmp, key, jmp);
    if (!ret.ok) {
        lisp_err(lisp, "redefined label: %s (%lx)", symbol->c, key);
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
static void lisp_defun(struct lisp *lisp, const struct symbol *symbol)
{
    lisp_label(lisp, symbol);
}


// -----------------------------------------------------------------------------
// implementation
// -----------------------------------------------------------------------------

typedef void (*lisp_fn_t) (struct lisp *);

static struct htable lisp_fn = {0};

static void lisp_register_fn(uint64_t key, lisp_fn_t fn)
{
    struct htable_ret ret = htable_put(&lisp_fn, key, (uintptr_t) fn);
    assert(ret.ok);
}

#include "vm/lisp_token.c"
#include "vm/lisp_fn.c"
#include "vm/lisp_asm.c"
#include "vm/lisp_disasm.c"


// -----------------------------------------------------------------------------
// interface
// -----------------------------------------------------------------------------

void mod_compiler_init(void)
{
    lisp_fn_register();
    lisp_asm_register();
}

struct mod *mod_compile(
        size_t len, const char *src, struct mods *mods, struct atoms *atoms)
{
    struct lisp lisp = {0};
    lisp.mods = mods;
    lisp.atoms = atoms;
    lisp.in.it = src;
    lisp.in.base = src;
    lisp.in.end = src + len;
    lisp.symb.fn = htable_clone(&lisp_fn);

    {
        lisp_stmts(&lisp);
        lisp_write_op(&lisp, OP_RESET);
    }

    for (struct htable_bucket *it = htable_next(&lisp.symb.req, NULL);
         it; it = htable_next(&lisp.symb.req, it))
    {
        lisp_err(&lisp, "unknown label: %lx", it->value);
        vec64_free((void *) it->value);
    }

    struct mod *mod = mod_alloc(
            lisp.in.base, lisp.in.end - lisp.in.base,
            lisp.out.base, lisp.out.it - lisp.out.base,
            lisp.err.list, lisp.err.len,
            lisp.index.list, lisp.index.len);

    free(lisp.out.base);
    free(lisp.err.list);
    free(lisp.index.list);
    htable_reset(&lisp.symb.fn);
    htable_reset(&lisp.symb.req);
    htable_reset(&lisp.symb.jmp);
    return mod;
}
