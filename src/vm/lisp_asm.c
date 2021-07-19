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
    case token_atom: { val = atoms_atom(lisp->atoms, &token->val.symb); break; }
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
    case token_symb: { val = lisp_reg(lisp, &token->val.symb); break; }
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

    int8_t val = token->val.num;
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

#define op_fn(op, arg)                                  \
    static void lisp_asm_ ## op(struct lisp *lisp)      \
    {                                                   \
        lisp_asm_ ## arg(lisp, OP_ ## op);              \
    }
#include "vm/op_xmacro.h"

static void lisp_asm_register(void)
{
    {
        struct symbol symbol = make_symbol_len(1, "@");
        lisp_register_fn(symbol_hash(&symbol), lisp_asm_label);
    }

#define op_fn(op, arg) \
    do {                                                                \
        struct symbol symbol = make_symbol_len(sizeof(#op), #op);       \
        lisp_register_fn(symbol_hash(&symbol), lisp_asm_ ## op);        \
    } while (false);

#include "vm/op_xmacro.h"

}
