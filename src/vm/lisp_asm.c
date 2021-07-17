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

static void lisp_asm_register(void)
{

#define register_asm(op) \
    lisp_register_fn(symb_hash_str(#op), (uintptr_t) lisp_fn_ ## op)

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

// -----------------------------------------------------------------------------
// disasm
// -----------------------------------------------------------------------------

struct disasm
{
    struct { const uint8_t *base, *end, *it; } in;

    struct text text;
    struct line *line;
};

static void lisp_disasm_outc(struct disasm *disasm, const char *str)
{
    line_setc(&disasm.text, disasm.line, strlen(str), str);
    disasm.line = text_insert(&disasm.text, disasm.line);
}

static void lisp_disasm_out(
        struct disasm *disasm, ip_t ip, const char *op, size_t arg_len, const char *arg)
{
    disasm.line->user = ip;

    if (!arg_len) line_setf(&disasm.text, disasm.line, 32, "  (%s)\n", op);
    else line_setf(&disasm.text, disasm.line, 32, "  (%s %s)\n", op, arg);

    disasm.line = text_insert(&disasm.text, disasm.line);
}

#define lisp_disasm_read_into(disasm, _ptr) ({                  \
    bool ok = disasm.in.it + sizeof(val) <= disasm.in.end;      \
    if (likely(ok)) {                                           \
        typeof(_ptr) ptr = (_ptr);                              \
        memcpy(ptr, disasm.in.it, sizeof(*ptr));                \
        disasm.in.it += sizeof(*ptr);                           \
    }                                                           \
    ok;
})

static ip_t lisp_disasm_ip(struct disasm *disasm)
{
    return (disasm.it - disasm.base) - 1;
}

static bool lisp_disasm_nil(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);
    lisp_disasm_out(disasm, ip, op, 0, NULL);
}

static bool lisp_disasm_lit(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    word_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[2+16] = {'0','x'};
    str_atox(val, buf+2, sizeof(buf)-2);

    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_len(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    uint8_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[3] = {0};
    str_atou(val, buf, sizeof(buf));
    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_reg(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    uint8_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[1+3] = {'$'};
    str_atou(val, buf+1, sizeof(buf)-1);
    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_off(struct disasm *disasm, const char *op)
{
    ip_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[2+8] = {'0','x'};
    str_atox(val, buf+2, sizeof(buf)-2);
    lisp_disasm_out(disasm, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_mod(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    ip_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[1+8] = {'0','x'};
    str_atox(val, buf+2, sizeof(buf)-2);
    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_op(struct disasm *disasm, enum op_code op)
{
    switch (op)
    {

#define disasm_op(op, arg) \
    case OP_ ## op: { return lisp_disasm_ ## arg (disasm, #op); }

    disasm_op(NOOP, nil)

    disasm_op(PUSH, lit)
    disasm_op(PUSHR, reg)
    disasm_op(PUSHF, nil)
    disasm_op(POP, nil)
    disasm_op(POPR, reg)
    disasm_op(DUPE, nil)
    disasm_op(SWAP, nil)
    disasm_op(ARG0, len)
    disasm_op(ARG1, len)
    disasm_op(ARG2, len)
    disasm_op(ARG3, len)

    disasm_op(NOT, nil)
    disasm_op(AND, nil)
    disasm_op(XOR, nil)
    disasm_op(OR, nil)
    disasm_op(BNOT, nil)
    disasm_op(BAND, nil)
    disasm_op(BXOR, nil)
    disasm_op(BOR, nil)
    disasm_op(BSL, nil)
    disasm_op(BSR, nil)

    disasm_op(NEG, nil)
    disasm_op(ADD, nil)
    disasm_op(SUB, nil)
    disasm_op(MUL, nil)
    disasm_op(MUL, nil)
    disasm_op(LMUL, nil)
    disasm_op(DIV, nil)
    disasm_op(REM, nil)

    disasm_op(EQ, nil)
    disasm_op(NE, nil)
    disasm_op(GT, nil)
    disasm_op(LT, nil)
    disasm_op(CMP, nil)

    disasm_op(RET, nil)
    disasm_op(CALL, mod)
    disasm_op(LOAD, mod)
    disasm_op(JMP, off)
    disasm_op(JZ, off)
    disasm_op(JNZ, off)

    disasm_op(RESET, nil)
    disasm_op(YIELD, nil)
    disasm_op(TSC, nil)
    disasm_op(FAULT, nil)

    disasm_op(IO, len)
    disasm_op(IOS, nil)
    disasm_op(IOR, reg)

    disasm_op(PACK, nil)
    disasm_op(UNPACK, nil)

#undef disasm_op

    default: { return false; }
    }
}

struct text mod_disasm(const struct mod *mod)
{
    struct disasm disasm = { 0 };

    disasm.in.it = mod->code;
    disasm.in.base = mod->code;
    disasm.in.end = mod->code + mod->len;

    text_init(&disasm.text);
    disasm.line = disasm.text.first;

    lisp_disasm_outc(dissam, "(asm");

    while (disasm.in.it < disasm.in.end) {
        if (!lisp_disasm_op(&disasm, *disasm.in.it)) goto fail;
    }

    lisp_disasm_outc(dissam, ")");

    return disasm.text;

  fail:
    text_clear(&disasm.text);
    return disasm.text;
}
