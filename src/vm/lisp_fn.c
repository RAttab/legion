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
        if (lisp->depth) lisp_err(lisp, "premature eof");
        return false;
    }

    case token_close: {
        lisp->depth--;
        return false;
    }
    case token_open: {
        lisp->depth++;
        struct token *token = lisp_expect(lisp, token_symb);
        lisp_index(lisp);

        struct htable_ret ret = {0};
        uint64_t key = token_symb_hash(token);

        if ((ret = htable_get(&lisp->symb.fn)).ok)
            ((lisp_fn_t) ret.value)(lisp);
        else lisp_fn_call(lisp);

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
        lisp_write_value(lisp, lisp_reg(lisp, &token->val.symb));
        return true;
    }

    default: { assert(false); }
    }
}

static void lisp_stmts(struct lisp *lisp)
{
    if (!lisp_stmt(lisp)) {
        // ensures that all statement return exactly one value on the stack.
        lisp_write_value(lisp, OP_PUSH);
        lisp_write_value(lisp, (word_t) 0);
        return;
    }

    while (lisp_stmt(lisp)) {
        lisp_write_value(lisp, OP_SWAP);
        lisp_write_value(lisp, OP_POP);
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
    if (args > 4) lisp_err(lisp, "too many arguments: %u > 4", (unsigned) args);

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
    if (lisp->depth) lisp_err(lisp, "nested function");

    // since we can have top level instructions, we don't want to actually
    // execute defun unless it's called so we instead jump over it.
    lisp_write_value(lisp, OP_JMP);
    ip_t skip = lisp_skip(lisp, sizeof(ip_t));

    {
        struct token *token = lisp_expect(lisp, token_symb);
        if (!token) { lisp_goto_close(lisp); return; }
        lisp_defun(lisp, &token->val.symb);
    }

    if (!lisp_expect(lisp, token_open)) { lisp_goto_close(lisp); return; }

    for (reg_t reg = 0; (token = lisp_next(lisp))->type != token_close; ++reg) {
        if (token->type != token_symb) {
            lisp_err(lisp, "unexpected token in argument list: %s != %s",
                    token_type_str(token->type), token_type_str(token_symb));
            lisp_goto_close(lisp);
            break;
        }

        if (reg >= 4) lisp_err(lisp, "too many parameters: %u > 4", reg);

        lisp->regs[reg] = token_symb_hash(token);
    }

    lisp_stmts(lisp);

    // our return value is above the return ip on the stack. Swaping the two
    // ensures that we can call RET and that the return value will be on top of
    // the stack after we return. MAGIC!
    lisp_write_value(lisp, OP_SWAP);

    lisp_write_value(lisp, OP_RET);
    lisp_write_value_at(lisp, skip, lisp_ip(lisp));

    // every statement must return a value on the stack even if it's at the
    // top-level.
    lisp_write_value(lisp, OP_PUSH);
    lisp_write_value(lisp, (word_t) 0);
}


// -----------------------------------------------------------------------------
// builtins
// -----------------------------------------------------------------------------

// pseudo function that essentially returns whatever is at the top of the stack.
// This is mostly useful when dealing with statements that can return multiple
// values.
static void lisp_fn_head(struct lisp *lisp) { (void) lisp; }

// when writting assembly, we won't follow the rule where every statement must
// return one value on the stack which means that if used in a typical stmts
// block then things will get poped out of the stack arbitrarily.
//
// the asm statement essentially applies all the included statement as is with
// zero stack modification in between. It's still expected that the overall
// block pushes one or more element on the stack in used within lisp statements
// but that's up to the user to manage.
static void lisp_fn_asm(struct lisp *lisp)
{
    while (lisp_stmt(lisp));
}

static void lisp_fn_let(struct lisp *lisp)
{
    struct token *token = lisp_expect(lisp, token_open);

    lisp_regs_t regs = {0};

    while ((token = lisp_next(lisp))) {
        if (token->type == token_close) break;
        if (token->type != token_open) {
            lisp_err(lisp, "unexpected token in argument list: %s != %s",
                    token_type_str(token->type), token_type_str(token_open));
            lisp_goto_close(lisp);
            break;
        }

        uint64_t key = token_symb_hash(lisp_expect(lisp, token_symb));

        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing statement");
            continue;
        }

        reg_t reg = lisp_reg_alloc(lisp, key);
        lisp_write_value(lisp, OP_POPR);
        lisp_write_value(lisp, reg);
        regs[reg] = key;

        if (!lisp_expect(lisp, token_close))
            lisp_goto_close(lisp);
    }

    lisp_stmts(lisp);

    for (reg_t reg = 0; reg < regs; ++reg)
        lisp_reg_free(lisp, reg, regs[reg]);
}

static void lisp_fn_if(struct lisp *lisp)
{
    if (!lisp_stmt(lisp)) { // predicate
        lisp_err(lisp, "missing predicate-clause");
        return;
    }

    lisp_write_value(lisp, OP_JZ);
    ip_t jmp_else = lisp_skip(lisp, sizeof(ip_t));

    if (!lisp_stmt(lisp)) { // true-clause
        lisp_err(lisp, "missing true-clause");
        return;
    }

    lisp_write_value(lisp, OP_JZ);
    ip_t jmp_end = lisp_skip(lisp, sizeof(ip_t));
    lisp_write_value_at(lisp, jmp_else, lisp_ip(lisp));

    if (!lisp_stmt(lisp)) { // else-clause (optional)
        // if we have no else clause and we're not executing the if clause then
        // this ensures that we always return a value on the stack.
        lisp_write_value(lisp, OP_PUSH);
        lisp_write_value(lisp, (word_t) 0);
    }

    lisp_write_value_at(lisp, jmp_end, lisp_ip(lisp));
}

static void lisp_fn_while(struct lisp *lisp)
{
    // init
    {
        // used to ensure that we always return one value on the stack
        lisp_write_value(lisp, OP_PUSH);
        lisp_write_value(lisp, (word_t) 0);
    }

    // predicate
    ip_t jmp_end = 0;
    ip_t jmp_pred = lisp_ip(lisp);
    {
        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing predicate-clause");
            return;
        }

        lisp_write_value(lisp, OP_JZ);
        jmp_end = lisp_skip(lisp, sizeof(ip_t));
    }

    // stmts
    {
        // we will always have a value on the stack before executing the stmt
        // block (see push at the start) so get rid of it.
        lisp_write_value(lisp, OP_POP);

        lisp_stmts(lisp); // loop-clause

        lisp_write_value(lisp, OP_JMP);
        lisp_write_value(lisp, jmp_pred);
    }

    lisp_write_value_at(lisp, jmp_end, lisp_it(lisp));
}

static void lisp_fn_for(struct lisp *lisp)
{
    reg_t reg = 0;
    uint64_t key = 0;

    // init
    {
        if (!lisp_expect(lisp, token_open)) {
            lisp_err(lisp, "missing init-clause");
            return;
        }

        struct token *token = lisp_expect(lisp, token_symb);
        if (!token) {
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }

        key = token_symb_hash(token);
        reg = lisp_reg_alloc(lisp, key);

        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing init-clause initializer statement");
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }

        lisp_write_value(lisp, OP_POPR);
        lisp_write_value(lisp, reg);

        if (!lisp_expect(lisp, token_close))
            lisp_goto_close(lisp);

        // used to ensure that we always return one value on the stack
        lisp_write_value(lisp, OP_PUSH);
        lisp_write_value(lisp, (word_t) 0);
    }

    // predicate
    ip_t jmp_end = 0;
    ip_t jmp_pred = lisp_ip(lisp);
    {
        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing predicate-clause");
            return;
        }

        lisp_write_value(lisp, OP_JZ);
        jmp_end = lisp_skip(lisp, sizeof(ip_t));
    }

    // inc
    {
        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing increment-clause");
            return;
        }

        lisp_write_value(lisp, OP_POPR);
        lisp_write_value(lisp, reg);
    }

    // stmts
    {
        // we will always have a value on the stack before executing the stmt
        // block (see push at the start) so get rid of it.
        lisp_write_value(lisp, OP_POP);

        lisp_stmts(lisp); // loop-clause

        lisp_write_value(lisp, OP_JMP);
        lisp_write_value(lisp, jmp_pred);
    }

    lisp_write_value_at(lisp, jmp_end, lisp_it(lisp));
    lisp_reg_free(lisp, reg, key);
}

static void lisp_fn_id(struct lisp *lisp)
{
    if (!lisp_stmt(lisp)) { // type
        lisp_err(lisp, "missing type argument");
        return;
    }

    lisp_write_value(lisp, (word_t) 24);
    lisp_write_value(lisp, OP_BSL);

    if (!lisp_stmt(lisp)) { // index
        lisp_err(lisp, "missing index argument");
        return;
    }

    lisp_write_value(lisp, OP_ADD);
}

// This function is awkward because we execute the argument in the reverse order
// that we want them. While we can fix the header, the actual arguments to the
// io command need to be written in reverse order. This is not really fixable
// without adding a vm instruction to do so :/
static void lisp_fn_io(struct lisp *lisp)
{
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing op argument"); return; }
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing dst argument"); return; }

    lisp_write_value(lisp, OP_SWAP);
    lisp_write_value(lisp, OP_PACK);

    size_t len = 0;
    while (lisp_stmt(lisp)) {
        lisp_write_value(lisp, OP_SWAP); // gotta keep the op at the top
        len++;
    }

    if (len > vm_io_cap)
        lisp_err(lisp, "too many io arguments: %zu > %u", len, vm_io_cap);

    lisp_write_value(lisp, OP_IO);
    lisp_write_value(lisp, (uint8_t) len);
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
    if (!lisp_stmt(lisp)) {
        lisp_err(lisp, "missing argument");
        return;
    }

    lisp_write_value(lisp, op);
}

static void lisp_fn_2(struct lisp *lisp, enum op_code op)
{
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing first argument"); return; }
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing second argument"); return; }
    lisp_write_value(lisp, op);
}

static void lisp_fn_n(struct lisp *lisp, enum op_code op)
{
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing first argument"); return; }
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing second argument"); return; }
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

static void lisp_fn_register(void)
{
#define register_fn(fn) \
    lisp_register_fn(symb_hash_str(#fn), (uintptr_t) lisp_fn_ ## fn)

#define register_fn_str(fn, str) \
    lisp_register_fn(symb_hash_str(str), (uintptr_t) lisp_fn_ ## fn)

    register_fn(defun);

    register_fn(head);
    register_fn(asm);
    register_fn(let);
    register_fn(if);
    register_fn(while);
    register_fn(for);

    register_fn(id);
    register_fn(io);

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

#undef register_fn_str
#undef register_fn
}
