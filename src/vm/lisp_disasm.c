/* lisp_disasm.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c

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
