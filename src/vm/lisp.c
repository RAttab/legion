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

static uint64_t symb_hash(symb_t *symb)
{
    return hash_str(&(symb[0]), symb_cap);
}

static uint64_t symb_hash_str(const char *str)
{
    assert(strlen(str) < symb_cap);

    symb_t symb = {0};
    strcpy(str, symb);
    return symb_hash(&symb);
}


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

typedef uint64_t lisp_regs_t[4];

struct lisp
{
    size_t depth;
    struct token token;

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
        struct htable fn;
        struct htable req;
        struct htable jmp;
        lisp_regs_t regs[4];
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

static uint64_t token_symb_hash(struct token *token)
{
    return symb_hash(&token->val.symb);
}

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

static void lisp_ensure(struct lisp *lisp, size_t len)
{
    if (likely(lisp->out.it + len <= lisp->out.end)) return;

    if (!lisp->out.base) lisp->out.base = calloc(page_len, 1);
    else {
        size_t pos = lisp->out.it - lisp->out.first;
        size_t cap = (lisp->out.end - lisp->out.base) * 2;
        lisp->out.base = realloc(lisp->out, cap);
        lisp->out.end = lisp->out.base + cap;
        lisp->out.it = lisp->out.base + pos;
    }
}

static void lisp_write(struct lisp *lisp, size_t len, uint8_t *data)
{

    memcpy(lisp->out.it, data, len);
}

#define lisp_write_value(lisp, _value)                  \
    do {                                                \
        typeof(_value) value = (_value);                \
        lisp_write(lisp, &value, sizeof(value));        \
    } while (false)


static void lisp_write_at(struct lisp *lisp, ip_t pos, size_t len, uint8_t *data)
{
    uint8_t *it = lisp->out.base + pos;
    assert(it + len < lisp->out.end);
    memcpy(it, data, len);
}

#define lisp_write_value_at(lisp, at, _value)           \
    do {                                                \
        typeof(_value) value = (_value);                \
        lisp_write_at(lisp, at, &value, sizeof(value)); \
    } while (false)


static ip_t lisp_skip(struct lisp *lisp, size_t len)
{
    lisp_ensure(lisp, len);

    ip_t pos = lisp->out.it - lisp->out.base;
    lisp->out.it += len;
    return pos;
}

static ip_t lisp_ip(struct lisp *lisp)
{
    return lisp->out.it - lisp->out.base;
}

static reg_t lisp_reg(struct lisp *lisp, uint64_t key)
{
    for (reg_t reg = 0; reg < 4; ++reg) {
        if (lisp->symb.reg[reg] == key) return reg;
    }
    abort();
}

static reg_t lisp_reg_alloc(struct lisp *lisp, uint64_t key)
{
    for (reg_t reg = 0; reg < 4; ++reg) {
        if (!lisp->symb.reg[reg]) {
            lisp->symb.reg[reg] = key;
            return reg;
        }
    }
    abort();
}

static void lisp_reg_free(struct lisp *lisp, reg_t reg, uint64_t key)
{
    if (!key) return;
    assert(lisp->symb.reg[reg] == key);
    lisp->symb.reg[reg] = 0;
}

static ip_t lisp_jmp(struct lisp *lisp, uint64_t key)
{
    struct htable_ret ret = htable_get(&lisp->symb.jmp, key);
    ip_t ip = ret.value;

    if (!ret.ok) {
        ret = htable_put(&lisp->symb.req, key, lisp_skip(lisp, sizeof(ip_t)));
        assert(ret.ok);
    }

    return ip;
}

typedef void (*lisp_fn_t) (struct lisp *);

static void lisp_register(struct lisp *lisp, uint64_t key, lisp_fn_t fn)
{
    uint64_t key = symb_hash_str(#fn);                              \
    uint64_t val = (uintptr_t) lisp_fn_ ## fn;                      \
    struct htable_ret ret = htable_put(&lisp->symb.fn, key, val);   \
    assert(ret.ok);                                                 \
}


#include "vm/lisp_asm.c"
#include "vm/lisp_fn.c"
