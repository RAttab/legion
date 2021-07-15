/* lisp_fn.c
   Rémi Attab (remi.attab@gmail.com), 15 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c


// -----------------------------------------------------------------------------
// declarations
// -----------------------------------------------------------------------------

void lisp_fn_call(struct lisp *lisp);


// -----------------------------------------------------------------------------
// stmt
// -----------------------------------------------------------------------------

static bool lisp_stmt(struct lisp *lisp)
{
    struct token *token = lisp_next(lisp);

    switch (token->type)
    {

    case token_nil: {
        if (lisp->depth) abort();
        return false;
    }

    case token_close: {
        lisp->depth--;
        return false;
    }
    case token_open: {
        lisp->depth++;
        struct token *token = lisp_expect(lisp, token_symb);

        struct htable_ret ret = {0};
        uint64_t key = token_symb_hash(token);

        if ((ret = htable_get(&lisp->symb.fn)).ok)
            ((lisp_fn_t) ret.value)(lisp);

        else if ((ret = htable_get(&lisp->symb.jmp)).ok)
            lisp_fn_call(lisp);

        else abort();

        return true;
    }

    case token_num: {
        lisp_write_value(lisp, OP_PUSH);
        lisp_write_value(lisp, token->val.num);
        return true;
    }
    case token_atom: {
        lisp_write_value(lisp, OP_PUSH);
        lisp_write_value(lisp, vm_atom(&token->val.symb));
        return true;
    }
    case token_reg: {
        lisp_write_value(lisp, OP_PUSHR);
        lisp_write_value(lisp, (uint8_t) token->val.num);
        return true;
    }
    case token_symb: {
        lisp_write_value(lisp, OP_PUSHR);
        lisp_write_value(lisp, lisp_reg(lisp, token_symb_hash(token)));
        return true;
    }

    default: { abort(); }
    }
}


// -----------------------------------------------------------------------------
// defun/call
// -----------------------------------------------------------------------------

static void lisp_fn_call(struct lisp *lisp)
{
    uint64_t key = token_symb_hash(lisp->token);

    reg_t args = 0;
    while (lisp_stmt(lisp)) ++args;
    if (args >= 4) abort();

    reg_t reg = 0;
    for (; reg < 4; ++reg) {
        bool has_arg = reg < args;
        bool has_reg = lisp->symb.reg[reg];

        if (has_arg && has_reg) {
            lisp_write_value(lisp, OP_ARG0 + reg);
            lisp_write_value(lisp, (reg_t) (args - reg - 1));
        }
        else if (has_arg && !hash_req) {
            lisp_write_value(lisp, OP_POPR);
            lisp_write_value(lisp, reg);
        }
        else if (!has_arg && hash_req) {
            lisp_write_value(lisp, OP_PUSHR);
            lisp_write_value(lisp, reg);
        }
    }

    lisp_write_value(lisp, OP_CALL);
    lisp_write_value(lisp, lisp_jmp(lisp, key));

    do {
        lisp_write_value(lisp, OP_SWAP); // ret value is on top of the stack
        lisp_write_value(lisp, OP_POPR);
        lisp_write_value(lisp, reg);
    } while (--reg);
}


static void lisp_fn_defun(struct lisp *lisp)
{
    if (lisp->depth != 1) abort();

    lisp_write_value(lisp, OP_JMP);
    ip_t skip = lisp_skip(lisp, sizeof(ip_t));

    {
        struct token *token = lisp_next(lisp);
        if (token->type != token_symb) abort();

        uint64_t key = token_symb_hash(token);
        struct htable_ret ret = htable_put(&lisp->symb.jmp, key, lisp_ip(lisp));
        if (!ret.ok) abort();
    }

    lisp_expect(lisp, token_open);
    for (reg_t reg = 0; (token = lisp_next(lisp))->type != token_close; ++reg) {
        if (token->type != token_symb) abort();
        if (reg >= 4) abort();
        lisp->regs[reg] = token_symb_hash(token);
    }

    while (lisp_stmt(lisp));

    lisp_write_value(lisp, OP_RET);
    lisp_write_value_at(lisp, skip, lisp_ip(lisp));
}


// -----------------------------------------------------------------------------
// builtins
// -----------------------------------------------------------------------------

// for funs that return multiple value on the stack
static void lisp_fn_head(struct lisp *lisp) { (void) lisp; }

static void lisp_fn_let(struct lisp *lisp)
{
    struct token *token = lisp_expect(lisp, token_open);

    lisp_regs_t regs = {0};

    while ((token = lisp_next(lisp))) {
        if (token->type == token_close) break;
        if (token->type != token_open) abort();

        uint64_t key = token_symb_hash(lisp_expect(lisp, token_symb));

        if (!lisp_stmt(lisp)) abort();

        reg_t reg = lisp_reg_alloc(lisp, key);
        lisp_write_value(lisp, OP_POPR);
        lisp_write_value(lisp, reg);
        regs[reg] = key;

        (void) lisp_expect(lisp, token_close);
    }

    while (lisp_stmt(lisp));

    for (reg_t reg = 0; reg < regs; ++reg)
        lisp_reg_free(lisp, reg, regs[reg]);
}

static void lisp_fn_if(struct list *lisp)
{
    if (!lisp_stmt(lisp)) abort(); // predicate

    lisp_write_value(lisp, OP_JZ);
    ip_t jmp_else = lisp_skip(lisp, sizeof(ip_t));

    if (!lisp_stmt(lisp)) abort(); // true-clause

    lisp_write_value(lisp, OP_JZ);
    ip_t jmp_end = lisp_skip(lisp, sizeof(ip_t));
    lisp_write_value_at(lisp, jmp_else, lisp_ip(lisp));

    (void) lisp_stmt(lisp); // else-clause (optional)

    lisp_write_value_at(lisp, jmp_end, lisp_ip(lisp));
}

static void lisp_fn_while(struct list *lisp)
{
    ip_t jmp_top = lisp_ip(lisp);

    if (!lisp_stmt(lisp)) abort(); // predictate

    lisp_write_value(lisp, OP_JZ);
    ip_t jmp_end = lisp_skip(lisp, sizeof(ip_t));

    while (lisp_stmt(lisp)); // loop-clause

    lisp_write_value(lisp, OP_JMP);
    lisp_write_value(lisp, jmp_top);

    lisp_write_value_at(lisp, jmp_end, lisp_it(lisp));
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

static void lisp_fn_0(struct lisp *lisp, enum op_code op)
{
    lisp_write_value(lisp, op);
}

static void lisp_fn_1(struct lisp *lisp, enum op_code op)
{
    if (!lisp_stmt(lisp)) abort();
    lisp_write_value(lisp, op);
}

static void lisp_fn_2(struct lisp *lisp, enum op_code op)
{
    if (!lisp_stmt(lisp)) abort();
    if (!lisp_stmt(lisp)) abort();
    lisp_write_value(lisp, op);
}

static void lisp_fn_n(struct lisp *lisp, enum op_code op)
{
    if (!lisp_stmt(lisp)) abort();
    if (!lisp_stmt(lisp)) abort();
    lisp_write_value(lisp, op);

    while (lisp_stmt(lisp)) lisp_write_value(lisp, op);
}

#define define_fn_ops(fn, op, arg)                      \
    static void lisp_fn_ ## fn(struct lisp *lisp)       \
    {                                                   \
        lisp_fn_ ## arg(lisp, OP_ ## op);               \
    }

    define_fn_ops(not, NOT, 1)
    define_fn_ops(and, AND, n)
    define_fn_ops(xor, XOR, n)
    define_fn_ops(or, OR, n)
    define_fn_ops(bnot, BNOT, 1)
    define_fn_ops(band, BAND, n)
    define_fn_ops(bxor, BXOR, n)
    define_fn_ops(bor, BOR, n)
    define_fn_ops(bsl, BSL, 2)
    define_fn_ops(bsr, BSR, 2)

    define_fn_ops(neg, NEG, 1)
    define_fn_ops(add, ADD, n)
    define_fn_ops(sub, SUB, 2)
    define_fn_ops(mul, MUL, 2)
    define_fn_ops(lmul, LMUL, 2)
    define_fn_ops(div, DIV, 2)
    define_fn_ops(rem, REM, 2)

    define_fn_ops(eq, EQ, 2)
    define_fn_ops(ne, NE, 2)
    define_fn_ops(gt, GT, 2)
    define_fn_ops(lt, LT, 2)
    define_fn_ops(cmp, CMP, 2)

    define_fn_ops(reset, RESET, 0)
    define_fn_ops(yield, YIELD, 0)
    define_fn_ops(tsc, TSC, 0)
    define_fn_ops(fault, FAULT, 0)

    define_fn_ops(pack, PACK, 2)
    define_fn_ops(unpack, UNPACK, 2)

#undef define_fn_ops


// -----------------------------------------------------------------------------
// index
// -----------------------------------------------------------------------------

static void lisp_register_fn(struct lisp *lisp)
{
#define register_fn(fn) \
    lisp_register(lisp, symb_hash_str(#fn), (uintptr_t) lisp_fn_ ## fn)

#define register_fn_str(fn, str) \
    lisp_register(lisp, symb_hash_str(str), (uintptr_t) lisp_fn_ ## fn)

    register_fn(defun);

    register_fn(head);
    register_fn(let);

    register_fn(if);
    register_fn(while);

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

    register_fn(neg);
    register_fn_str(add, "+");
    register_fn_str(sub, "-");
    register_fn_str(mul, "*");
    register_fn(lmul);
    register_fn_str(div, "/");
    register_fn(rem);
    register_fn_str(rem, "mod");

    register_fn_str(eq, "=");
    register_fn_str(ne, "/=");
    register_fn_str(gt, ">");
    register_fn_str(lt, "<");
    register_fn(cmp);

    register_fn(reset);
    register_fn(yield);
    register_fn(tsc);
    register_fn(fault);

    register_fn(pack);
    register_fn(unpack);

#undef register_fn
#undef register_fn_str
}
