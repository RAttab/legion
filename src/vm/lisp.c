/* lisp.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

struct lisp
{
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

    size_t depth;
    struct htable symb;
    struct token token;
};

// -----------------------------------------------------------------------------
// regs
// -----------------------------------------------------------------------------

typedef uint8_t reg_t;
typedef uint8_t regs_t;
static const reg_t reg_nil = 4;

static size_t regs_alloc(regs_t *regs)
{
    size_t index = u64_ctz(*regs);
    if (index >= reg_nil) return reg_nil;

    *regs |= 1 << index;
    return index;
}


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

enum { symb_cap = 16 };
typedef char symb_t[symb_cap];

enum token_type
{
    token_nil = 0,
    token_stmt,
    token_atom,
    token_symb,
    token_num,
    token_end,
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
static void lisp_in_inc(struct lisp *lisp)
{
    if (*lisp->in.it == '\n') { lisp->in.col++; lisp->in.row = 0; }
    else { lisp->in.row++; }
    lisp->in.it++;
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
    case ')': { lisp->token.type = token_end; break; }
    case '(': { lisp->token.type = token_stmt; break; }
    case '!': { lisp->token.type = token_atom; break; }
    case '@': { lisp->token.type = token_symb; break; }
    case '0'...'9': { lisp->token.type = token_num; break; }
    default: { abort(); }
    }

    switch (lisp->token.type)
    {

    case token_stmt:
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

#define lisp_write_from(lisp, _ptr)                     \
    do {                                                \
        typeof(_ptr) ptr = (_ptr);                      \
        lisp_write(lisp, ptr, sizeof(*ptr));            \
    } while (false)


static void lisp_parse(struct lisp *lisp)
{
    struct token *token = lisp_next(lisp);

}
