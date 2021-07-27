/* lisp_disasm.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c

#include "utils/text.h"


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
    line_setc(&disasm->text, disasm->line, strlen(str), str);
    disasm->line = text_insert(&disasm->text, disasm->line);
}

static void lisp_disasm_out(
        struct disasm *disasm, ip_t ip, const char *op, size_t arg_len, const char *arg)
{
    disasm->line->user = ip;

    if (!arg_len) line_setf(&disasm->text, disasm->line, 32, "  (%s)", op);
    else line_setf(&disasm->text, disasm->line, 32, "  (%s %s)", op, arg);

    disasm->line = text_insert(&disasm->text, disasm->line);
}

#define lisp_disasm_read_into(_dasm, _ptr)                      \
    ({                                                          \
        struct disasm *dasm = (_dasm);                          \
        typeof(_ptr) ptr = (_ptr);                              \
                                                                \
        bool ok = dasm->in.it + sizeof(*ptr) <= dasm->in.end;   \
        if (likely(ok)) {                                       \
            memcpy(ptr, dasm->in.it, sizeof(*ptr));             \
            dasm->in.it += sizeof(*ptr);                        \
        }                                                       \
        ok;                                                     \
    })

static ip_t lisp_disasm_ip(struct disasm *disasm)
{
    return (disasm->in.it - disasm->in.base) - 1;
}

static bool lisp_disasm_nil(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);
    lisp_disasm_out(disasm, ip, op, 0, NULL);
    return true;
}

static bool lisp_disasm_lit(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    word_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[2+16+1] = {0}; buf[0] = '0'; buf[1] ='x';
    str_utox(val, buf+2, sizeof(buf)-2-1);

    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_len(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    uint8_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[3+1] = {0};
    str_utoa(val, buf, sizeof(buf)-1);

    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_reg(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    uint8_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[1+3+1] = {0}; buf[0] = '$';
    str_utoa(val, buf+1, sizeof(buf)-1-1);

    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_off(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    ip_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[2+8+1] = {0}; buf[0] = '0'; buf[1] = 'x';
    str_utox(val, buf+2, sizeof(buf)-2-1);

    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_mod(struct disasm *disasm, const char *op)
{
    ip_t ip = lisp_disasm_ip(disasm);

    word_t val = 0;
    if (!lisp_disasm_read_into(disasm, &val)) return false;

    char buf[2+16+1] = {0}; buf[0] = '0'; buf[1] = 'x';
    str_utox(val, buf+2, sizeof(buf)-2-1);

    lisp_disasm_out(disasm, ip, op, sizeof(buf), buf);
    return true;
}

static bool lisp_disasm_op(struct disasm *disasm, enum op_code op)
{
    switch (op)
    {

#define op_fn(op, arg) \
    case OP_ ## op: { return lisp_disasm_ ## arg (disasm, #op); }
#include "vm/op_xmacro.h"

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

    lisp_disasm_outc(&disasm, "(asm");

    while (disasm.in.it < disasm.in.end) {
        enum op_code op = 0;
        if (!lisp_disasm_read_into(&disasm, &op)) goto fail;
        if (!lisp_disasm_op(&disasm, op)) goto fail;
    }

    lisp_disasm_outc(&disasm, ")");

    return disasm.text;

  fail:
    text_clear(&disasm.text);
    return disasm.text;
}
