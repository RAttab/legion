/* lisp_fn.c
   Rémi Attab (remi.attab@gmail.com), 15 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c

// -----------------------------------------------------------------------------
// fn
// -----------------------------------------------------------------------------

static void lisp_fn_call(struct lisp *lisp)
{
    uint64_t key = symb_hash(&lisp->token.val.symb);

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
    lisp_regs_t ctx = legion_xchg(lisp->symb.regs, (lisp_regs_t) {0});

    {
        lisp_write_value(lisp, OP_CALL);

        struct htable_ret ret = htable_get(&lisp->symb.jmp, key);
        if (ret.ok) lisp_write_value(lisp, (ip_t) ret.value);
        else {
            ret = htable_put(&lisp->symb.req, key, lisp_skip(lisp, sizeof(ip_t)));
            assert(ret.ok);
        }
    }

    lisp->symb.regs = ctx;
    do {
        lisp_write_value(lisp, OP_SWAP); // ret value is on top of the stack
        lisp_write_value(lisp, OP_POPR);
        lisp_write_value(lisp, reg);
    } while (--reg);
}

static void lisp_fn_fun(struct lisp *lisp)
{
    if (lisp->depth != 1) abort();

    lisp_write_value(lisp, OP_JMP);
    ip_t skip = lisp_skip(lisp, sizeof(ip_t));

    {
        struct token *token = lisp_next(lisp);
        if (token->type != token_symb) abort();

        uint64_t key = symb_hash(&token->val.symb);
        struct htable_ret ret = htable_put(&lisp->symb.jmp, key, lisp_ip(lisp));
        if (!ret.ok) abort();
    }

    lisp_expect(lisp, token_open);
    for (reg_t reg = 0; (token = lisp_next(lisp))->type != token_close; ++reg) {
        if (token->type != token_symb) abort();
        if (reg >= 4) abort();
        lisp->regs[reg] = symb_hash(&token->val.symb);
    }

    while (lisp_stmt(lisp));

    lisp_write_value(lisp, OP_RET);
    lisp_write_value_at(lisp, skip, lisp_ip(lisp));
}

static void lisp_fn_add(struct lisp *lisp)
{
    if (!lisp_parse(lisp)) abort();
    if (!lisp_parse(lisp)) abort();
    lisp_write_value(lisp, OP_ADD);
}
