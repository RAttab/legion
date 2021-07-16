/* lisp_asm.c
   Rémi Attab (remi.attab@gmail.com), 15 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c


// -----------------------------------------------------------------------------
// templates
// -----------------------------------------------------------------------------

static void lisp_asm_nil(struct lisp *lisp, enum op_code op)
{
    lisp_write_value(lisp, op);
}

static void lisp_asm_lit(struct lisp *lisp, enum op_code op)
{
    struct token *token = lisp_next(lisp);

    word_t val = 0;
    switch (token->type) {
    case token_num: { val = token->val.num; break; }
    case token_atom: { val = vm_atom(&token->val.symb); break; }
    default: {
        lisp_err(lisp, "unexpected token: %s != %s | %s",
                token_type_str(token->type),
                token_type_str(token_num),
                token_type_str(token_atom));
        return;
    }
    }

    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);
}

static void lisp_asm_reg(struct lisp *lisp, enum op_code op)
{
    struct token *token = lisp_next(lisp);

    reg_t val = 0;
    switch (token->type) {
    case token_reg: { val = token->val.num; break; }
    case token_symb: { val = lisp_reg(lisp, token_symb_hash(token)); break; }
    default: {
        lisp_err(lisp, "unexpected token: %s != %s | %s",
                token_type_str(token->type),
                token_type_str(token_reg),
                token_type_str(token_symb));
        return;
    }
    }

    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);
}

static void lisp_asm_len(struct lisp *lisp, enum op_code op)
{
    struct token *token = lisp_expect(lisp, token_num);
    if (!token) return;

    if (token->val.num > 0x7F) {
        lisp_err(lisp, "invalid length argument: %zu > %u", token->val.num, 0x7F);
        return;
    }

    int8_t len = token->val.num;
    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);
}

static void lisp_asm_off(struct lisp *lisp, enum op_code op)
{
    struct token *token = lisp_next(lisp);

    ip_t val = 0;
    switch (token->type) {
    case token_num: { val = token->val.num; break; }
    case token_symb: { val = lisp_jmp(lisp, token_symb_hash(token)); break; }
    default: {
        lisp_err(lisp, "unexpected token: %s != %s | %s",
                token_type_str(token->type),
                token_type_str(token_num),
                token_type_str(token_symb));
        return;
    }
    }

    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);
}

static void lisp_asm_mod(struct lisp *lisp, enum op_code op)
{
    struct token *token = lisp_expect(lisp, token_symb);
    if (!token) return;

    word_t val = lisp_jmp(lisp, token_symb_hash(token));
    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

static void lisp_asm_label(struct lisp *lisp)
{
    struct token *token = lisp_expect(lisp, token_symb);
    if (!token) return;

    lisp_label(lisp, &token->val.symb);
}

#define define_asm(op, arg)                             \
    static void lisp_asm_ ## op(struct lisp *lisp)      \
    {                                                   \
        lisp_asm_ ## arg(lisp, OP_ ## op);              \
    }

    define_asm(NOOP, nil)

    define_asm(PUSH, lit)
    define_asm(PUSHR, reg)
    define_asm(PUSHF, nil)
    define_asm(POP, nil)
    define_asm(POPR, reg)
    define_asm(DUPE, nil)
    define_asm(SWAP, nil)
    define_asm(ARG0, len)
    define_asm(ARG1, len)
    define_asm(ARG2, len)
    define_asm(ARG3, len)

    define_asm(NOT, nil)
    define_asm(AND, nil)
    define_asm(XOR, nil)
    define_asm(OR, nil)
    define_asm(BNOT, nil)
    define_asm(BAND, nil)
    define_asm(BXOR, nil)
    define_asm(BOR, nil)
    define_asm(BSL, nil)
    define_asm(BSR, nil)

    define_asm(NEG, nil)
    define_asm(ADD, nil)
    define_asm(SUB, nil)
    define_asm(MUL, nil)
    define_asm(MUL, nil)
    define_asm(LMUL, nil)
    define_asm(DIV, nil)
    define_asm(REM, nil)

    define_asm(EQ, nil)
    define_asm(NE, nil)
    define_asm(GT, nil)
    define_asm(LT, nil)
    define_asm(CMP, nil)

    define_asm(RET, nil)
    define_asm(CALL, mod)
    define_asm(LOAD, mod)
    define_asm(JMP, off)
    define_asm(JZ, off)
    define_asm(JNZ, off)

    define_asm(RESET, nil)
    define_asm(YIELD, nil)
    define_asm(TSC, nil)
    define_asm(FAULT, nil)

    define_asm(IO, len)
    define_asm(IOS, nil)
    define_asm(IOR, reg)

    define_asm(PACK, nil)
    define_asm(UNPACK, nil)

#undef define_asm

static void lisp_register_asm(struct lisp *lisp)
{

#define register_asm(op) \
    lisp_register_fn(lisp, symb_hash_str(#op), (uintptr_t) lisp_fn_ ## op)

    register_asm(NOOP);

    register_asm(PUSH);
    register_asm(PUSHR);
    register_asm(PUSHF);
    register_asm(POP);
    register_asm(POPR);
    register_asm(DUPE);
    register_asm(SWAP);
    register_asm(ARG0);
    register_asm(ARG1);
    register_asm(ARG2);
    register_asm(ARG3);

    register_asm(NOT);
    register_asm(AND);
    register_asm(XOR);
    register_asm(OR);
    register_asm(BNOT);
    register_asm(BAND);
    register_asm(BXOR);
    register_asm(BOR);
    register_asm(BSL);
    register_asm(BSR);

    register_asm(NEG);
    register_asm(ADD);
    register_asm(SUB);
    register_asm(MUL);
    register_asm(MUL);
    register_asm(LMUL);
    register_asm(DIV);
    register_asm(REM);

    register_asm(EQ);
    register_asm(NE);
    register_asm(GT);
    register_asm(LT);
    register_asm(CMP);

    register_asm(RET);
    register_asm(CALL);
    register_asm(LOAD);
    register_asm(JMP);
    register_asm(JZ);
    register_asm(JNZ);

    register_asm(RESET);
    register_asm(YIELD);
    register_asm(TSC);
    register_asm(FAULT);

    register_asm(IO);
    register_asm(IOS);
    register_asm(IOR);

    register_asm(PACK);
    register_asm(UNPACK);

#undef register_asm
}
