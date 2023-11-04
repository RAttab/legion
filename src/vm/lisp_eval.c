/* lisp_eval.c
   Rémi Attab (remi.attab@gmail.com), 30 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c

// -----------------------------------------------------------------------------
// index
// -----------------------------------------------------------------------------

typedef vm_word (*lisp_eval_fn) (struct lisp *);

static struct htable lisp_fn_eval = {0};


// -----------------------------------------------------------------------------
// eval
// -----------------------------------------------------------------------------

static vm_word lisp_eval(struct lisp *lisp)
{
    struct token *token = lisp_next(lisp);
    switch (token->type)
    {

    case token_nil: {
        lisp_err(lisp, "premature eof");
        return 0;
    }

    case token_close: {
        lisp_err(lisp, "unexpected close token");
        lisp->depth--;
        return 0;
    }

    case token_open: {
        lisp->depth++;
        struct token *token = lisp_expect(lisp, token_symbol);

        uint64_t key = symbol_hash(&token->value.s);
        struct htable_ret ret = htable_get(&lisp_fn_eval, key);
        if (!ret.ok) {
            lisp_err(lisp, "invalid constant function: %s", token->value.s.c);
            return 0;
        }

        vm_word value = ((lisp_eval_fn) ret.value)(lisp);
        lisp_assert_close(lisp, lisp_next(lisp));
        return value;
    }

    case token_number: { return token->value.w; }
    case token_symbol: { return lisp_const(lisp, &token->value.s); }

    case token_atom: {
        vm_word value = atoms_get(lisp->atoms, &token->value.s);
        if (!value) lisp_err(lisp, "unregistered atom '%s'", token->value.s.c);
        return value;
    }
    case token_atom_make: {
        return atoms_make(lisp->atoms, &token->value.s);
    }

    case token_reg: {
        lisp_err(lisp, "invalid register reference in const expression");
        return 0;
    }

    default: { assert(false); }
    }
}

static void lisp_eval_goto_close(struct lisp *lisp)
{
    while (!lisp_eof(lisp) && !lisp_peek_close(lisp)) lisp_next(lisp);
}


// -----------------------------------------------------------------------------
// fn
// -----------------------------------------------------------------------------

static vm_word lisp_eval_if(struct lisp *lisp)
{
    vm_word pred = lisp_eval(lisp);
    vm_word branch_true = lisp_eval(lisp);
    vm_word branch_false = lisp_peek_close(lisp) ? 0 : lisp_eval(lisp);
    return pred ? branch_true : branch_false;
}

static vm_word lisp_eval_when(struct lisp *lisp)
{
    vm_word pred = lisp_eval(lisp);
    vm_word value = lisp_eval(lisp);
    return pred ? value : 0;
}

static vm_word lisp_eval_unless(struct lisp *lisp)
{
    vm_word pred = lisp_eval(lisp);
    vm_word value = lisp_eval(lisp);
    return pred ? 0 : value;
}

// Keep in sync with lisp_fn_mod
static vm_word lisp_eval_mod(struct lisp *lisp)
{
    mod_id id = 0;
    struct token peek = {0};

    if (lisp_peek(lisp, &peek)->type != token_close)
        id = lisp_parse_mod(lisp);

    else { // self-reference
        const struct mod *mod = mods_latest(lisp->mods, lisp->mod_maj);
        id = make_mod(lisp->mod_maj, mod_version(mod->id) + 1);
    }

    if (!id) { lisp_eval_goto_close(lisp); return 0; }
    return id;
}

// Need to ensure that the op is done on u64 to avoid shifting into the sign
// bit.
static vm_word lisp_eval_bsl(struct lisp *lisp)
{
    uint64_t value = lisp_eval(lisp);
    while (!lisp_peek_close(lisp))
        value = value << lisp_eval(lisp);
    return value;
}

// Need to ensure that the op is done on u64 to avoid issues with sign
// extension.
static vm_word lisp_eval_bsr(struct lisp *lisp)
{
    uint64_t value = lisp_eval(lisp);
    while (!lisp_peek_close(lisp))
        value = value >> lisp_eval(lisp);
    return value;
}

static vm_word lisp_eval_sub(struct lisp *lisp)
{
    vm_word value = lisp_eval(lisp);
    if (lisp_peek_close(lisp)) return -value;

    while (!lisp_peek_close(lisp)) value -= lisp_eval(lisp);
    return value;
}

static vm_word lisp_eval_cmp(struct lisp *lisp)
{
    return lisp_eval(lisp) - lisp_eval(lisp);
}

static vm_word lisp_eval_pack(struct lisp *lisp)
{
    vm_word x = lisp_eval(lisp);
    if (x < 0 || x > UINT32_MAX)
        lisp_err(lisp, "argument would be truncated: %lx", x);

    vm_word y = lisp_eval(lisp);
    if (y < 0 || y > UINT32_MAX)
        lisp_err(lisp, "argument would be truncated: %lx", y);

    return vm_pack(y, x);
}

static vm_word lisp_eval_id(struct lisp *lisp)
{
    vm_word type = lisp_eval(lisp);
    if (type < 0 || type > items_max)
        lisp_err(lisp, "invalid item type: %lx", type);

    vm_word seq = lisp_eval(lisp);
    if (seq < 0 || seq >= (1U << 24))
        lisp_err(lisp, "invalid id sequence number: %lx", seq);

    return make_im_id(type, seq);
}

static vm_word lisp_eval_specs(struct lisp *lisp)
{
    vm_word spec = lisp_eval(lisp);
    if (!spec_validate(spec))
        lisp_err(lisp, "invalid spec id: %lx", spec);

    struct token token = {0};
    vm_word args[specs_max_args] = {0};
    size_t len = 0;

    for (; lisp_peek(lisp, &token)->type != token_close; ++len) {
        vm_word arg = lisp_eval(lisp);
        if (len < specs_max_args) args[len] = arg;
        else lisp_err(lisp, "too many arguments for specs");
    }

    struct specs_ret ret = specs_args(spec_from_word(spec), args, len);
    if (!ret.ok) lisp_err(lisp, "invalid specs expression");
    return ret.word;
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

#define define_eval_ops_1(fn, op)                       \
    static vm_word lisp_eval_ ## fn(struct lisp *lisp)  \
    {                                                   \
        return op lisp_eval(lisp);                      \
    }

#define define_eval_ops_2(fn, op)                       \
    static vm_word lisp_eval_ ## fn(struct lisp *lisp)  \
    {                                                   \
        return lisp_eval(lisp) op lisp_eval(lisp);      \
    }

// Gotta be careful about short-circuit boolean ops.
#define define_eval_ops_n(fn, op)                       \
    static vm_word lisp_eval_ ## fn(struct lisp *lisp)  \
    {                                                   \
        vm_word value = lisp_eval(lisp);                \
        while (!lisp_peek_close(lisp)) {                \
            vm_word arg = lisp_eval(lisp);              \
            value = value op arg;                       \
        }                                               \
        return value;                                   \
    }

    define_eval_ops_1(not,  !)
    define_eval_ops_n(and,  &&)
    define_eval_ops_n(or,   ||)
    define_eval_ops_n(xor,  ^)
    define_eval_ops_1(bnot, ~)
    define_eval_ops_n(band, &)
    define_eval_ops_n(bor,  |)
    define_eval_ops_n(bxor, ^)
    // bsl -> lisp_eval_bsl
    // bsr -> lisp_eval_bsr

    define_eval_ops_n(add, +)
    // sub -> lisp_eval_sub
    define_eval_ops_n(mul, *)
    define_eval_ops_2(div, /)
    define_eval_ops_2(rem, %)

    define_eval_ops_2(eq, ==)
    define_eval_ops_2(ne, !=)
    define_eval_ops_2(gt, >)
    define_eval_ops_2(ge, >=)
    define_eval_ops_2(lt, <)
    define_eval_ops_2(le, <=)

#undef define_eval_ops_1
#undef define_eval_ops_2
#undef define_eval_ops_n


// -----------------------------------------------------------------------------
// index
// -----------------------------------------------------------------------------

static void lisp_register_eval(uint64_t key, lisp_eval_fn eval)
{
    struct htable_ret ret = htable_put(&lisp_fn_eval, key, (uintptr_t) eval);
    assert(ret.ok);
}

static void lisp_eval_register(void)
{
#define register_fn(fn)                                                 \
    do {                                                                \
        struct symbol symbol = make_symbol_len(#fn, sizeof(#fn));       \
        lisp_register_eval(symbol_hash(&symbol), lisp_eval_ ## fn);     \
    } while (false)

#define register_fn_str(fn, str)                                        \
    do {                                                                \
        struct symbol symbol = make_symbol(str);                        \
        lisp_register_eval(symbol_hash(&symbol), lisp_eval_ ## fn);     \
    } while (false)

    register_fn(if);
    register_fn(when);
    register_fn(unless);
    register_fn(mod);

    register_fn(not);
    register_fn(and);
    register_fn(xor);
    register_fn(or);
    register_fn(bnot);
    register_fn(band);
    register_fn(bxor);
    register_fn(bor);
    register_fn(bsl);
    register_fn(bsr);

    register_fn_str(add, "+");
    register_fn_str(sub, "-");
    register_fn_str(mul, "*");
    register_fn_str(div, "/");
    register_fn(rem);

    register_fn_str(eq, "=");
    register_fn_str(ne, "/=");
    register_fn_str(gt, ">");
    register_fn_str(ge, ">=");
    register_fn_str(lt, "<");
    register_fn_str(le, "<=");
    register_fn(cmp);

    register_fn(pack);
    register_fn(id);
    register_fn(specs);

#undef register_fn_str
#undef register_fn
}
