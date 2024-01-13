/* lisp.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// lisp
// -----------------------------------------------------------------------------

struct lisp_req
{
    struct lisp_req *next;

    uint32_t pos, len;
    vm_ip ip;
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
    /* dbgf("err: %u:%u %s", err->pos, err->len, err->str); */

    if (unlikely(lisp->err.len == lisp->err.cap)) {
        size_t old = lisp->err.cap;
        lisp->err.cap = lisp->err.cap ? lisp->err.cap * 2 : 8;
        lisp->err.list = mem_array_realloc_t(
                lisp->err.list, *lisp->err.list, old, lisp->err.cap);
    }
    lisp->err.len++;

    struct mod_err *start = lisp->err.list;
    struct mod_err *it = &lisp->err.list[lisp->err.len-1];
    while (it > start) {
        struct mod_err *prev = it - 1;
        if (prev->pos < err->pos) break;
        *it = *prev;
        it = prev;
    }

    *it = *err;
}

legion_printf(4, 5)
static void lisp_err_at(
        struct lisp *lisp, uint32_t pos, size_t len, const char *fmt, ...)
{
    struct mod_err err = { .pos = pos, .len = len };

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
        .pos = lisp->token.pos,
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
        .pos = lisp->token.pos,
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

static void lisp_goto_close(struct lisp *lisp, bool check_token)
{
    token_goto_close(&lisp->in, check_token ? &lisp->token : nullptr);
}

static struct token *lisp_next(struct lisp *lisp)
{
    return token_next(&lisp->in, &lisp->token);
}

static struct token *lisp_peek(struct lisp *lisp, struct token *token)
{
    return token_peek(&lisp->in, token);
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
        lisp_goto_close(lisp, false);
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

    size_t old = cap;
    cap = cap ? cap * 2 : sys_page_len;

    lisp->out.base = mem_realloc(lisp->out.base, old, cap);
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
static void lisp_write_op(struct lisp *lisp, enum vm_op op)
{
    lisp_write(lisp, &op, sizeof(op));
}

#define lisp_write_value(lisp, _value)                  \
    do {                                                \
        typeof(_value) v = (_value);                    \
        lisp_write(lisp, &v, sizeof(v));                \
    } while (false)


static void lisp_write_at(
        struct lisp *lisp, vm_ip pos, const void *data, size_t len)
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


static vm_ip lisp_skip(struct lisp *lisp, size_t len)
{
    lisp_ensure(lisp, len);

    vm_ip pos = lisp->out.it - lisp->out.base;
    lisp->out.it += len;
    return pos;
}

static vm_ip lisp_ip(struct lisp *lisp)
{
    return lisp->out.it - lisp->out.base;
}


// -----------------------------------------------------------------------------
// index
// -----------------------------------------------------------------------------

static void lisp_index_at(struct lisp *lisp, const struct token *token)
{
    vm_ip ip = lisp_ip(lisp);
    size_t prev = lisp->index.len - 1;

    struct mod_index *index = NULL;
    if (lisp->index.len && lisp->index.list[prev].ip == ip)
        index = &lisp->index.list[prev];
    else {
        if (lisp->index.len == lisp->index.cap) {
            size_t old = lisp->index.cap;
            lisp->index.cap = lisp->index.cap ? lisp->index.cap * 2 : 8;
            lisp->index.list = mem_array_realloc_t(
                    lisp->index.list, *lisp->index.list, old, lisp->index.cap);
        }

        index = &lisp->index.list[lisp->index.len];
        lisp->index.len++;
    }

    index->pos = token->pos;
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
    for (vm_reg reg = 0; reg < 4; ++reg) {
        if (lisp->symb.regs[reg] == key) return true;
    }
    return false;
}

static vm_reg lisp_reg(struct lisp *lisp, const struct symbol *symbol)
{
    uint64_t key = symbol_hash(symbol);
    for (vm_reg reg = 0; reg < 4; ++reg) {
        if (lisp->symb.regs[reg] == key) return reg;
    }

    lisp_err(lisp, "unknown symbol: %s (%lx)", symbol->c, key);
    return 0;
}

static vm_reg lisp_reg_alloc(struct lisp *lisp, uint64_t key)
{
    for (vm_reg reg = 0; reg < 4; ++reg) {
        if (!lisp->symb.regs[reg]) {
            lisp->symb.regs[reg] = key;
            return reg;
        }
    }

    lisp_err(lisp, "no available registers to allocate");
    return 0;
}

static void lisp_reg_free(struct lisp *lisp, vm_reg reg, uint64_t key)
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

static vm_word lisp_const(struct lisp *lisp, const struct symbol *symbol)
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

    struct symbol *value = mem_alloc_t(value);
    *value = *symbol;

    ret = htable_put(&lisp->pub.symb, key, (uintptr_t) value);
    assert(ret.ok);
}

static void lisp_publish(struct lisp *lisp, const struct symbol *symbol, vm_ip ip)
{
    if (unlikely(lisp->pub.len == lisp->pub.cap)) {
        size_t old = lisp->pub.cap;
        lisp->pub.cap = lisp->pub.cap ? lisp->pub.cap * 2 : 2;
        lisp->pub.list = mem_array_realloc_t(
                lisp->pub.list, *lisp->pub.list, old, lisp->pub.cap);
    }

    uint64_t key = symbol_hash(symbol);
    lisp->pub.list[lisp->pub.len] = (struct mod_pub) { .key = key, .ip = ip };
    lisp->pub.len++;

    lisp_pub_symbol(lisp, symbol);
}


// -----------------------------------------------------------------------------
// jmp
// -----------------------------------------------------------------------------

static vm_ip lisp_jmp(struct lisp *lisp, const struct token *token)
{
    assert(token->type == token_symbol);

    const struct symbol *symbol = &token->value.s;
    uint64_t key = symbol_hash(symbol);

    struct htable_ret ret = htable_get(&lisp->symb.jmp, key);
    if (likely(ret.ok)) return ret.value;

    ret = htable_get(&lisp->symb.req, key);

    struct lisp_req *old = ret.ok ? (void *) ret.value : NULL;
    struct lisp_req *new = mem_alloc_t(new);
    *new = (struct lisp_req) {
        .next = old,
        .ip = lisp_ip(lisp),
        .pos = token->pos,
        .len = token->len,
    };

    if (!old)
        ret = htable_put(&lisp->symb.req, key, (uintptr_t) new);
    else if (old != new)
        ret = htable_xchg(&lisp->symb.req, key, (uintptr_t) new);
    assert(ret.ok);

    // If we're jumping to a function that doesn't exist we need to symbol to
    // print a proper error message.
    lisp_pub_symbol(lisp, symbol);
    return 0;
}

static void lisp_label(struct lisp *lisp, const struct symbol *symbol)
{
    vm_ip jmp = lisp_ip(lisp);

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
        mem_free(req);
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

        struct htable_ret ret = htable_get(&lisp->pub.symb, key);
        assert(ret.ok);

        while (req) {
            struct symbol *symbol = (void *) ret.value;
            lisp_err_at(lisp, req->pos, req->len,
                    "unknown function or label: %s (%lx)", symbol->c, it->value);

            struct lisp_req *next = req->next;
            mem_free(req);
            req = next;
        }
    }
}


// -----------------------------------------------------------------------------
// parse
// -----------------------------------------------------------------------------

static mod_maj lisp_mods_find(
        struct lisp *lisp, const struct symbol *name)
{
    if (lisp->mods) return mods_find(lisp->mods, name);

    assert(lisp->mods_list);

    uint64_t hash = symbol_hash(name);
    struct mods_item *it = lisp->mods_list->items;
    const struct mods_item *end = it + lisp->mods_list->len;
    for (; it < end; ++it) {
        if (symbol_hash(&it->str) == hash) return it->maj;
    }

    return 0;
}

static mod_id lisp_mods_latest(struct lisp *lisp, mod_maj maj)
{
    if (lisp->mods) {
        const struct mod *mod = mods_latest(lisp->mods, lisp->mod_maj);
        return mod ? mod->id : 0;
    }

    assert(lisp->mods_list);

    struct mods_item *it = lisp->mods_list->items;
    const struct mods_item *end = it + lisp->mods_list->len;
    for (; it < end; ++it) {
        if (it->maj == maj) return make_mod(maj, it->ver);
    }

    return 0;
}

// Supported syntax: mod[.ver]
static mod_id lisp_parse_mod(struct lisp *lisp)
{
    if (!lisp_expect(lisp, token_symbol)) return 0;
    struct token first = lisp->token;

    struct token peek = {0};
    struct token *second = NULL;
    if (lisp_peek(lisp, &peek)->type == token_sep) {
        assert(lisp_expect(lisp, token_sep));

        second = lisp_expect(lisp, token_number);
        if (!second) return 0;
        if (second->value.w > UINT16_MAX) {
            lisp_err(lisp, "invalid mod version: %ld", second->value.w);
            return 0;
        }
    }

    mod_maj mod_maj = lisp_mods_find(lisp, &first.value.s);
    if (!mod_maj) {
        lisp_err(lisp, "unknown mod: %s", first.value.s.c);
        return 0;
    }

    // We don't check if the version exists because we allow the reference
    // to mods that will be created. It's the only way to break circular
    // dependencies at the moment.
    return second ?
        make_mod(mod_maj, second->value.w) :
        lisp_mods_latest(lisp, mod_maj);
}


struct lisp_fun_ret { bool ok; bool local; vm_word jmp; };

// Supported syntax: [mod.[ver.]]fun
static struct lisp_fun_ret lisp_parse_fun(struct lisp *lisp, struct token *token)
{
    struct token first = *token;
    assert(first.type == token_symbol);

    struct token peek = {0};
    if (lisp_peek(lisp, &peek)->type != token_sep)
        return (struct lisp_fun_ret) { .ok = true, .local = true };
    assert(lisp_expect(lisp, token_sep));

    struct token second = *lisp_next(lisp);
    if (second.type != token_symbol && second.type != token_number) {
        lisp_err(lisp, "unexpected token type: %s", token_type_str(second.type));
        return (struct lisp_fun_ret) { .ok = false };
    }


    struct token *third = NULL;
    if (lisp_peek(lisp, &peek)->type == token_sep) {
        if (second.type != token_number) {
            lisp_err(lisp, "unexpected token type: %s", token_type_str(second.type));
            return (struct lisp_fun_ret) { .ok = false };
        }

        if (second.value.w > UINT16_MAX) {
            lisp_err(lisp, "invalid mod version: %ld", second.value.w);
            return (struct lisp_fun_ret) { .ok = false };
        }

        assert(lisp_expect(lisp, token_sep));

        third = lisp_expect(lisp, token_symbol);
        if (!third) return (struct lisp_fun_ret) { .ok = false };
    }


    const struct mod *mod = NULL;
    {
        mod_maj mod_maj = mods_find(lisp->mods, &first.value.s);
        if (!mod_maj) {
            lisp_err(lisp, "unknown mod: %s", first.value.s.c);
            return (struct lisp_fun_ret) { .ok = false };
        }

        if (third)
            mod = mods_get(lisp->mods, make_mod(mod_maj, second.value.w));
        else mod = mods_latest(lisp->mods, mod_maj);

        // Unlike lisp_parse_mod, we do check that the mod version exists here
        // because jump IP are tied to specific mod versions.
        if (!mod) {
            if (third)
                lisp_err(lisp, "unknown mod: %s.%ld", first.value.s.c, second.value.w);
            else lisp_err(lisp, "unknown mod: %s", first.value.s.c);
            return (struct lisp_fun_ret) { .ok = false };
        }
    }


    struct token *fun = third ? third : &second;

    vm_ip ip = mod_pub(mod, symbol_hash(&fun->value.s));
    if (ip == MOD_PUB_UNKNOWN) {
        lisp_err(lisp, "unknown fun '%s' in mod '%s.%u'",
                fun->value.s.c, first.value.s.c, mod_version(mod->id));
        return (struct lisp_fun_ret) { .ok = false };
    }


    return (struct lisp_fun_ret) {
        .ok = true,
        .jmp = vm_pack(mod->id, ip),
    };
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
        lisp_write_op(&lisp, vm_op_reset);
        lisp_index(&lisp);
    }

    lisp_label_unknown(&lisp);

    struct mod *mod = mod_alloc(
            lisp.in.base, lisp.in.end - lisp.in.base,
            lisp.out.base, lisp.out.it - lisp.out.base,
            lisp.pub.list, lisp.pub.len,
            lisp.err.list, lisp.err.len,
            lisp.index.list, lisp.index.len);

    mem_free(lisp.out.base);
    mem_free(lisp.err.list);
    mem_free(lisp.index.list);
    mem_free(lisp.pub.list);
    htable_reset(&lisp.consts);
    htable_reset(&lisp.symb.fn);
    htable_reset(&lisp.symb.req);
    htable_reset(&lisp.symb.jmp);

    for (const struct htable_bucket *it = htable_next(&lisp.pub.symb, NULL);
         it; it = htable_next(&lisp.pub.symb, it))
        mem_free((struct symbol *) it->value);
    htable_reset(&lisp.pub.symb);

    return mod;
}


// -----------------------------------------------------------------------------
// eval
// -----------------------------------------------------------------------------

struct lisp *lisp_new(struct mods_list *mods, struct atoms *atoms)
{
    struct lisp *lisp = mem_alloc_t(lisp);
    lisp->mods_list = mods;
    lisp->atoms = atoms;
    return lisp;
}

void lisp_free(struct lisp *lisp)
{
    mem_free(lisp);
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

    vm_word result = lisp_eval(lisp);
    for (size_t i = 0; i < lisp->err.len; ++i) {
        struct mod_err *err = lisp->err.list + i;
        dbgf("eval:%u: %s", err->pos, err->str);
    }

    return (struct lisp_ret) {
        .ok = lisp->err.len == 0,
        .value = result,
    };
}
