/* lisp.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/str.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// symb
// -----------------------------------------------------------------------------

enum { symb_cap = 16 };
typedef char symb_t[symb_cap];
uint64_t symb_hash(symb_t *symb) { return hash_str(&(symb[0]), symb_cap); }


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

struct lisp
{
    size_t depth;
    struct token token;
    struct htable fn;

    struct
    {
        size_t row, col;
        const char *const base;
        const char *const end;
        const char *it;
    } in;

    struct
    {
        uint8_t *base;
        uint8_t *end;
        uint8_t *it;
    } out;

    struct
    {
        struct htable req;
        struct htable jmp;
        uint64_t regs[4];
    } symb;
};


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

enum token_type
{
    token_nil = 0,
    token_open,
    token_close,
    token_atom,
    token_symb,
    token_num,
    token_reg,
};

struct token
{
    union { symb_t symb; word_t num; } val;
    enum token_type type;
};

static bool lisp_is_space(char c) { return c <= 0x20; }
static bool lisp_is_symb(char c)
{
    switch (c) {
    case '-':
    case 'a'...'z':
    case 'A'...'Z':
    case '0'...'9': return true;
    default: return false;
    }
}
static bool lisp_is_num(char c)
{
    switch (c) {
    case 'x':
    case '0'...'9': return true;
    default: return false;
    }
}


static bool lisp_eof(struct lisp *lisp) { return lisp->in.it >= lisp->in.end; }
static char lisp_in_inc(struct lisp *lisp)
{
    if (*lisp->in.it == '\n') { lisp->in.col++; lisp->in.row = 0; }
    else { lisp->in.row++; }

    char c = *lisp->in.it;
    lisp->in.it++;
    return c;
}

static void lisp_skip_spaces(struct lisp *lisp)
{
    while (!lisp_eof(lisp) && lisp_is_space(*lisp->in.it)) lisp_in_inc(lisp);
}

static struct token *lisp_next(struct lisp *lisp)
{
    lisp_skip_spaces(lisp);
    if (lisp_eof(lisp)) return token_nil;

    switch (*lisp->in.it) {
    case '(': { lisp->token.type = token_open; break; }
    case ')': { lisp->token.type = token_close; break; }
    case '!': { lisp->token.type = token_atom; break; }
    case '$': { lisp->token.type = token_reg; break; }
    case '0'...'9': { lisp->token.type = token_num; break; }
    default: {
        if (!lisp_is_symb(*lisp->in.it)) abort();
        lisp->token.type = token_sumb; break;
    }
    }

    switch (lisp->token.type)
    {

    case token_atom:
    case token_symb: {
        char *first = lisp->in.it;
        while(!lisp_eof(lisp) && lisp_is_symb(*lisp->in.it)) lisp_in_inc(lisp);

        size_t len = lisp->in.it - first;
        if (len > symb_cap) abort();
        memcpy(lisp->token.val.symb, first, len);

        if (!lisp_is_space(*lisp->in.it)) abort();
        break;
    }

    case token_num: {
        char *first = lisp->in.it;
        while (!lisp_eof(lisp) && lisp_is_num(*lisp->in.it)) lisp_in_inc(lisp);

        size_t len = lisp->in.it - first;
        size_t read = 0;
        if (len > 2 && first[0] == '0' && first[1] == 'x')
            read = str_atox(lisp->in.it+2, len-2, &lisp->token.val.num) + 2;
        else read = str_atou(lisp->in.it, len, &lisp->token.val.num);

        if (read != len) abort();
        if (!lisp_is_space(*lisp->in.it)) abort();
        break;
    }

    case token_reg: {
        if (lisp_eof(lisp)) abort();

        char c = lisp_in_inc(lisp);
        if (c < '1' && c > '4') abort();
        lisp->token.val.num = c - '1';

        if (!lisp_is_space(*lisp->in.it)) abort();
        break;
    }

    default: { break; }
    }

    return &lisp->token;
}

static bool lisp_expect(struct lisp *lisp, enum token_type exp)
{
    if (lisp_next(lisp)->type != exp) abort();
}


// -----------------------------------------------------------------------------
// parse
// -----------------------------------------------------------------------------

static void lisp_write(struct lisp *lisp, size_t len, uint8_t *data)
{
    if (unlikely(lisp->out.it + len >= lisp->out.end)) {
        if (!lisp->out.base) lisp->out.base = calloc(page_len, 1);
        else {
            size_t pos = lisp->out.it - lisp->out.first;
            size_t cap = (lisp->out.end - lisp->out.base) * 2;
            lisp->out.base = realloc(lisp->out, cap);
            lisp->out.end = lisp->out.base + cap;
            lisp->out.it = lisp->out.base + pos;
        }
    }

    memcpy(lisp->out.it, data, len);
}

#define lisp_write_value(lisp, _value)                  \
    do {                                                \
        typeof(_value) value = (_value);                \
        lisp_write(lisp, &value, sizeof(value));        \
    } while (false)


typedef void (*lisp_fn_t) (struct lisp *);

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

        struct htable_ret ret = htable_get(&lisp->fn, symb_hash(&token->val.symb));
        if (!ret.ok) abort();

        ((lisp_fn_t) ret.value)(lisp);
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
        reg_t reg = lisp_reg(lisp, symb_hash(&token->val.symb));
        if (!reg) abort();

        lisp_write_value(lisp, OP_PUSHR);
        lisp_write_value(lisp, reg);
        return true;
    }

    default: { abort(); }
    }
}


// -----------------------------------------------------------------------------
// fn
// -----------------------------------------------------------------------------

static void lisp_fn_fun(struct lisp *lisp)
{
    // name
    struct token *token = lisp_next(lisp);
    if (token->type != token_symb) abort();

    lisp_expect(lisp, token_open);
    while ((token = lisp_next(lisp))->type != token_close) {
        if (token->type != token_symb) abort();

        // arguments;
    }

    while (lisp_stmt(lisp));
}

static void lisp_fn_add(struct lisp *lisp)
{
    if (!lisp_parse(lisp)) abort();
    if (!lisp_parse(lisp)) abort();
    lisp_write_value(lisp, OP_ADD);
}
