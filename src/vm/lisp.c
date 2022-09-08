/* lisp.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/mod.h"
#include "vm/token.h"
#include "vm/atoms.h"
#include "vm/symbol.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/htable.h"

#include <stdarg.h>


// -----------------------------------------------------------------------------
// lisp
// -----------------------------------------------------------------------------

struct lisp_req
{
    struct lisp_req *next;
    ip ip;

    uint16_t row, col, len;
};

struct lisp
{
    mod_maj mod_maj;
    struct mods *mods;
    struct mods_list *mods_list;
    struct atoms *atoms;

    size_t depth;

    struct token token;
    struct tokenizer in;

    struct
    {
        uint8_t *base;
        uint8_t *end;
        uint8_t *it;
    } out;

    struct htable consts;

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
        struct mod_pub *list;
        struct htable symb;
    } pub;

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
// err
// -----------------------------------------------------------------------------

static void lisp_err_ins(struct lisp *lisp, struct mod_err *err)
{
    /* dbgf("err: %u:%u:%u %s", err->row, err->col, err->len, err->str); */

    if (unlikely(lisp->err.len == lisp->err.cap)) {
        lisp->err.cap = lisp->err.cap ? lisp->err.cap * 2 : 8;
        lisp->err.list = realloc(lisp->err.list, lisp->err.cap * sizeof(*lisp->err.list));
    }
    lisp->err.len++;

    struct mod_err *start = lisp->err.list;
    struct mod_err *it = &lisp->err.list[lisp->err.len-1];
    while (it > start) {
        struct mod_err *prev = it - 1;
        if (prev->row < err->row || (prev->row == err->row && prev->col < err->col))
            break;

        *it = *prev;
        it = prev;
    }

    *it = *err;
}

legion_printf(5, 6)
static void lisp_err_at(
        struct lisp *lisp, size_t row, size_t col, size_t len, const char *fmt, ...)
{
    struct mod_err err = { .row = row, .col = col, .len = len };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(err.str, mod_err_cap, fmt, args);
    va_end(args);

    lisp_err_ins(lisp, &err);
}

legion_printf(2, 3)
static void lisp_err(struct lisp *lisp, const char *fmt, ...)
{
    struct mod_err err = {
        .row = lisp->token.row,
        .col = lisp->token.col,
        .len = lisp->token.len,
    };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(err.str, mod_err_cap, fmt, args);
    va_end(args);

    lisp_err_ins(lisp, &err);
}

legion_printf(2, 3)
static void lisp_err_token(void *ctx, const char *fmt, ...)
{
    struct lisp *lisp = ctx;

    struct mod_err err = {
        .row = lisp->token.row,
        .col = lisp->token.col,
        .len = lisp->token.len,
    };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(err.str, mod_err_cap, fmt, args);
    va_end(args);

    lisp_err_ins(lisp, &err);
}


// -----------------------------------------------------------------------------
// in
// -----------------------------------------------------------------------------

static bool lisp_eof(struct lisp *lisp)
{
    return token_eof(&lisp->in);
}

static void lisp_goto_close(struct lisp *lisp)
{
    token_goto_close(&lisp->in);
}

static struct token *lisp_next(struct lisp *lisp)
{
    return token_next(&lisp->in, &lisp->token);
}

static struct token *lisp_assert_token(
        struct lisp *lisp, struct token *token, enum token_type exp)
{
    return token_assert(&lisp->in, token, exp);
}

static struct token *lisp_expect(struct lisp *lisp, enum token_type exp)
{
    return token_expect(&lisp->in, &lisp->token, exp);
}

static void lisp_assert_close(struct lisp *lisp, struct token *token)
{
    if (!lisp_assert_token(lisp, token, token_close))
        lisp_goto_close(lisp);
    lisp->depth--;
}

static void lisp_expect_close(struct lisp *lisp)
{
    lisp_assert_close(lisp, lisp_next(lisp));
}

static bool lisp_peek_close(struct lisp *lisp)
{
    return token_peek(&lisp->in, &lisp->token)->type == token_close;
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

static void lisp_write(struct lisp *lisp, const void *data, size_t len)
{
    lisp_ensure(lisp, len);
    memcpy(lisp->out.it, data, len);
    lisp->out.it += len;
}

// `sizeof(enum op_code) != sizeof(OP_XXX)` so gotta do it manually.
static void lisp_write_op(struct lisp *lisp, enum op_code op)
{
    lisp_write(lisp, &op, sizeof(op));
}

#define lisp_write_value(lisp, _value)                  \
    do {                                                \
        typeof(_value) v = (_value);                    \
        lisp_write(lisp, &v, sizeof(v));                \
    } while (false)


static void lisp_write_at(
        struct lisp *lisp, ip pos, const void *data, size_t len)
{
    uint8_t *it = lisp->out.base + pos;
    assert(it + len < lisp->out.end);
    memcpy(it, data, len);
}

#define lisp_write_value_at(lisp, at, _value)   \
    do {                                        \
        typeof(_value) v = (_value);            \
        lisp_write_at(lisp, at, &v, sizeof(v)); \
    } while (false)


static ip lisp_skip(struct lisp *lisp, size_t len)
{
    lisp_ensure(lisp, len);

    ip pos = lisp->out.it - lisp->out.base;
    lisp->out.it += len;
    return pos;
}

static ip lisp_ip(struct lisp *lisp)
{
    return lisp->out.it - lisp->out.base;
}


// -----------------------------------------------------------------------------
// index
// -----------------------------------------------------------------------------

static void lisp_index_at(struct lisp *lisp, const struct token *token)
{
    ip ip = lisp_ip(lisp);
    size_t prev = lisp->index.len - 1;

    struct mod_index *index = NULL;
    if (lisp->index.len && lisp->index.list[prev].ip == ip)
        index = &lisp->index.list[prev];
    else {
        if (lisp->index.len == lisp->index.cap) {
            lisp->index.cap = lisp->index.cap ? lisp->index.cap * 2 : 8;
            lisp->index.list = realloc(
                    lisp->index.list, lisp->index.cap * sizeof(*lisp->index.list));
        }

        index = &lisp->index.list[lisp->index.len];
        lisp->index.len++;
    }

    index->row = token->row;
    index->col = token->col;
    index->len = token->len;
    index->ip = lisp_ip(lisp);
}

static void lisp_index(struct lisp *lisp)
{
    lisp_index_at(lisp, &lisp->token);
}


// -----------------------------------------------------------------------------
// regs
// -----------------------------------------------------------------------------

static bool lisp_is_reg(struct lisp *lisp, const struct symbol *symbol)
{
    uint64_t key = symbol_hash(symbol);
    for (reg reg = 0; reg < 4; ++reg) {
        if (lisp->symb.regs[reg] == key) return true;
    }
    return false;
}

static reg lisp_reg(struct lisp *lisp, const struct symbol *symbol)
{
    uint64_t key = symbol_hash(symbol);
    for (reg reg = 0; reg < 4; ++reg) {
        if (lisp->symb.regs[reg] == key) return reg;
    }

    lisp_err(lisp, "unknown symbol: %s (%lx)", symbol->c, key);
    return 0;
}

static reg lisp_reg_alloc(struct lisp *lisp, uint64_t key)
{
    for (reg reg = 0; reg < 4; ++reg) {
        if (!lisp->symb.regs[reg]) {
            lisp->symb.regs[reg] = key;
            return reg;
        }
    }

    lisp_err(lisp, "no available registers to allocate");
    return 0;
}

static void lisp_reg_free(struct lisp *lisp, reg reg, uint64_t key)
{
    if (!key) return;

    if (lisp->symb.regs[reg] != key) {
        lisp_err(lisp, "register mismatch: %u -> %lx != %lx",
                reg, lisp->symb.regs[reg], key);
        return;
    }

    lisp->symb.regs[reg] = 0;
}


// -----------------------------------------------------------------------------
// consts
// -----------------------------------------------------------------------------

static word lisp_const(struct lisp *lisp, const struct symbol *symbol)
{
    uint64_t key = symbol_hash(symbol);
    struct htable_ret ret = htable_get(&lisp->consts, key);
    if (!ret.ok) lisp_err(lisp, "unknown constant: %s", symbol->c);
    return ret.ok ? ret.value : 0;
}


// -----------------------------------------------------------------------------
// pub
// -----------------------------------------------------------------------------

static void lisp_pub_symbol(struct lisp *lisp, const struct symbol *symbol)
{
    uint64_t key = symbol_hash(symbol);

    struct htable_ret ret = htable_get(&lisp->pub.symb, key);
    if (ret.ok) return;

    struct symbol *value = calloc(1, sizeof(*value));
    *value = *symbol;

    ret = htable_put(&lisp->pub.symb, key, (uintptr_t) value);
    assert(ret.ok);
}

static void lisp_publish(struct lisp *lisp, const struct symbol *symbol, ip ip)
{
    if (unlikely(lisp->pub.len == lisp->pub.cap)) {
        lisp->pub.cap = lisp->pub.cap ? lisp->pub.cap * 2 : 2;
        lisp->pub.list = realloc(lisp->pub.list, lisp->pub.cap * sizeof(*lisp->pub.list));
    }

    uint64_t key = symbol_hash(symbol);
    lisp->pub.list[lisp->pub.len] = (struct mod_pub) { .key = key, .ip = ip };
    lisp->pub.len++;

    lisp_pub_symbol(lisp, symbol);
}


// -----------------------------------------------------------------------------
// jmp
// -----------------------------------------------------------------------------

static ip lisp_jmp(struct lisp *lisp, const struct token *token)
{
    assert(token->type == token_symbol);

    const struct symbol *symbol = &token->value.s;
    uint64_t key = symbol_hash(symbol);

    struct htable_ret ret = htable_get(&lisp->symb.jmp, key);
    if (likely(ret.ok)) return ret.value;

    ret = htable_get(&lisp->symb.req, key);

    struct lisp_req *old = ret.ok ? (void *) ret.value : NULL;
    struct lisp_req *new = calloc(1, sizeof(*new));
    *new = (struct lisp_req) {
        .next = old,
        .ip = lisp_ip(lisp),
        .row = token->row,
        .col = token->col,
        .len = token->len,
    };

    if (!old)
        ret = htable_put(&lisp->symb.req, key, (uintptr_t) new);
    else if (old != new)
        ret = htable_xchg(&lisp->symb.req, key, (uintptr_t) new);
    assert(ret.ok);

    lisp_pub_symbol(lisp, symbol);
    return 0;
}

static void lisp_label(struct lisp *lisp, const struct symbol *symbol)
{
    ip jmp = lisp_ip(lisp);

    uint64_t key = symbol_hash(symbol);
    struct htable_ret ret = htable_put(&lisp->symb.jmp, key, jmp);
    if (!ret.ok) {
        lisp_err(lisp, "redefined label: %s (%lx)", symbol->c, key);
        return;
    }

    lisp_publish(lisp, symbol, jmp);

    ret = htable_get(&lisp->symb.req, key);
    if (!ret.ok) return;

    struct lisp_req *req = (void *) ret.value;
    while (req) {
        lisp_write_value_at(lisp, req->ip, jmp);

        struct lisp_req *next = req->next;
        free(req);
        req = next;
    }

    ret = htable_del(&lisp->symb.req, key);
    assert(ret.ok);
}

static void lisp_label_unknown(struct lisp *lisp)
{

    for (const struct htable_bucket *it = htable_next(&lisp->symb.req, NULL);
         it; it = htable_next(&lisp->symb.req, it))
    {
        uint64_t key = it->key;
        struct lisp_req *req = (void *) it->value;

        while (req) {
            struct htable_ret ret = htable_get(&lisp->pub.symb, key);
            assert(ret.ok);

            struct symbol *symbol = (void *) ret.value;
            lisp_err_at(lisp, req->row, req->col, req->len,
                    "unknown function or label: %s (%lx)", symbol->c, it->value);

            struct lisp_req *next = req->next;
            free(req);
            req = next;
        }
    }
}


// -----------------------------------------------------------------------------
// implementation
// -----------------------------------------------------------------------------

typedef void (*lisp_fn) (struct lisp *);

static struct htable lisp_fns = {0};

static void lisp_register_fn(uint64_t key, lisp_fn fn)
{
    struct htable_ret ret = htable_put(&lisp_fns, key, (uintptr_t) fn);
    assert(ret.ok);
}

#include "vm/lisp_eval.c"
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
    lisp_eval_register();
}

struct mod *mod_compile(
        mod_maj mod_maj,
        const char *src, size_t len,
        struct mods *mods, struct atoms *atoms)
{
    struct lisp lisp = {0};
    lisp.mod_maj = mod_maj;
    lisp.mods = mods;
    lisp.atoms = atoms;
    lisp.symb.fn = htable_clone(&lisp_fns);
    token_init(&lisp.in, src, len, lisp_err_token, &lisp);

    {
        lisp_stmts(&lisp);

        lisp_index(&lisp);
        lisp_write_op(&lisp, OP_RESET);
        lisp_index(&lisp);
    }

    lisp_label_unknown(&lisp);

    struct mod *mod = mod_alloc(
            lisp.in.base, lisp.in.end - lisp.in.base,
            lisp.out.base, lisp.out.it - lisp.out.base,
            lisp.pub.list, lisp.pub.len,
            lisp.err.list, lisp.err.len,
            lisp.index.list, lisp.index.len);

    free(lisp.out.base);
    free(lisp.err.list);
    free(lisp.index.list);
    free(lisp.pub.list);
    htable_reset(&lisp.consts);
    htable_reset(&lisp.symb.fn);
    htable_reset(&lisp.symb.req);
    htable_reset(&lisp.symb.jmp);

    for (const struct htable_bucket *it = htable_next(&lisp.pub.symb, NULL);
         it; it = htable_next(&lisp.pub.symb, it))
        free((struct symbol *) it->value);
    htable_reset(&lisp.pub.symb);

    return mod;
}


// -----------------------------------------------------------------------------
// eval
// -----------------------------------------------------------------------------

struct lisp *lisp_new(struct mods_list *mods, struct atoms *atoms)
{
    struct lisp *lisp = calloc(1, sizeof(*lisp));
    lisp->mods_list = mods;
    lisp->atoms = atoms;
    return lisp;
}

void lisp_free(struct lisp *lisp)
{
    free(lisp);
}

void lisp_context(struct lisp *lisp, struct mods_list *mods, struct atoms *atoms)
{
    lisp->mods_list = mods;
    lisp->atoms = atoms;
}

struct lisp_ret lisp_eval_const(struct lisp *lisp, const char *src, size_t len)
{
    token_init(&lisp->in, src, len, lisp_err_token, lisp);
    lisp->err.len = 0;

    word result = lisp_eval(lisp);
    for (size_t i = 0; i < lisp->err.len; ++i) {
        struct mod_err *err = lisp->err.list + i;
        dbgf("eval:%u:%u: %s", err->row, err->col, err->str);
    }

    return (struct lisp_ret) {
        .ok = lisp->err.len == 0,
        .value = result,
    };
}
