/* lisp_asm.c
   Rémi Attab (remi.attab@gmail.com), 15 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c


// -----------------------------------------------------------------------------
// templates
// -----------------------------------------------------------------------------

static void lisp_asm_nil(struct lisp *lisp, enum vm_op op)
{
    lisp_write_value(lisp, op);
    lisp_expect_close(lisp);
}

static void lisp_asm_lit(struct lisp *lisp, enum vm_op op)
{
    struct token *token = lisp_next(lisp);

    vm_word val = 0;
    switch (token->type)
    {

    case token_number: { val = token->value.w; break; }

    case token_atom: {
        val = atoms_get(lisp->atoms, &token->value.s);
        if (!val) {
            lisp_err(lisp, "unregistered atom '%s'", token->value.s.c);
            return;
        }
        break;
    }

    case token_atom_make: {
        val = atoms_make(lisp->atoms, &token->value.s);
        break;
    }

    default: {
        lisp_err(lisp, "unexpected token: %s != %s | %s",
                token_type_str(token->type),
                token_type_str(token_number),
                token_type_str(token_atom));
        lisp_goto_close(lisp, true);
        return;
    }
    }

    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);

    lisp_expect_close(lisp);
}

static void lisp_asm_reg(struct lisp *lisp, enum vm_op op)
{
    struct token *token = lisp_next(lisp);

    vm_reg val = 0;
    switch (token->type) {
    case token_reg: { val = token->value.w; break; }
    case token_symbol: { val = lisp_reg(lisp, &token->value.s); break; }
    default: {
        lisp_err(lisp, "unexpected token: %s != %s | %s",
                token_type_str(token->type),
                token_type_str(token_reg),
                token_type_str(token_symbol));
        lisp_goto_close(lisp, true);
        return;
    }
    }

    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);

    lisp_expect_close(lisp);
}

static void lisp_asm_len(struct lisp *lisp, enum vm_op op)
{
    struct token *token = lisp_expect(lisp, token_number);
    if (!token) return;

    if (token->value.w > 0x7F) {
        lisp_err(lisp, "invalid length argument: %zu > %u", token->value.w, 0x7F);
        lisp_goto_close(lisp, true);
        return;
    }

    int8_t val = token->value.w;
    lisp_write_value(lisp, op);
    lisp_write_value(lisp, val);

    lisp_expect_close(lisp);
}

static void lisp_asm_off(struct lisp *lisp, enum vm_op op)
{
    lisp_write_value(lisp, op);

    vm_ip val = 0;
    struct token *token = lisp_next(lisp);

    switch (token->type) {
    case token_number: { val = token->value.w; break; }
    case token_symbol: { val = lisp_jmp(lisp, token); break; }
    default: {
        lisp_err(lisp, "unexpected token: %s != %s | %s",
                token_type_str(token->type),
                token_type_str(token_number),
                token_type_str(token_symbol));
        lisp_goto_close(lisp, true);
        return;
    }
    }

    lisp_write_value(lisp, val);
    lisp_expect_close(lisp);
}

static void lisp_asm_mod(struct lisp *lisp, enum vm_op op)
{
    lisp_write_value(lisp, op);

    struct token sym = *lisp_next(lisp);
    struct lisp_fun_ret fun = lisp_parse_fun(lisp, &sym);
    if (!fun.ok) { lisp_goto_close(lisp, true); return; }

    if (!fun.local) lisp_write_value(lisp, fun.jmp);
    else if (lisp->token.type == token_symbol) {
        // little-endian flips the byte ordering... fuck me...
        lisp_write_value(lisp, lisp_jmp(lisp, &sym));
        lisp_write_value(lisp, (mod_id) 0);
    }

    lisp_expect_close(lisp);
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

static void lisp_asm_label(struct lisp *lisp)
{
    struct token *token = lisp_expect(lisp, token_symbol);
    if (!token) { lisp_goto_close(lisp, true); return; }

    lisp_label(lisp, &token->value.s);

    lisp_expect_close(lisp);
}

#define vm_op_fn(op, str, arg)                          \
    static void lisp_asm_ ## str(struct lisp *lisp)     \
    {                                                   \
        lisp_asm_ ## arg(lisp, op);                     \
    }
#include "vm/opx.h"

static void lisp_asm_register(void)
{
    {
        struct symbol symbol = make_symbol_len("@", 1);
        lisp_register_fn(symbol_hash(&symbol), lisp_asm_label);
    }

#define vm_op_fn(op, str, arg)                                          \
    do {                                                                \
        struct symbol symbol = make_symbol_len(#str, sizeof(#str));     \
        lisp_register_fn(symbol_hash(&symbol), lisp_asm_ ## str);       \
    } while (false);
#include "vm/opx.h"

}
