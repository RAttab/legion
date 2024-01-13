/* token.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/token.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// token_type
// -----------------------------------------------------------------------------

const char *token_type_str(enum token_type type)
{
    switch (type) {
    case token_nil: { return "eof"; }
    case token_open: { return "open"; }
    case token_close: { return "close"; }
    case token_atom: { return "atom"; }
    case token_atom_make: { return "atom_make"; }
    case token_symbol: { return "symbol"; }
    case token_number: { return "number"; }
    case token_reg: { return "register"; }
    case token_sep: { return "separator"; }
    case token_comment: { return "comment"; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// tokenizer
// -----------------------------------------------------------------------------

void token_init(
        struct tokenizer *tok,
        const char *src, size_t len,
        token_err_fn fn, void *ctx)
{
    assert(src);

    *tok = (struct tokenizer) {
        .row = 0,
        .col = 0,

        .base = src,
        .it = src,
        .end = src + len,

        .comments = false,

        .err_fn = fn,
        .err_ctx = ctx,
    };
}

void token_comments(struct tokenizer *tok, bool enable)
{
    tok->comments = enable;
}

bool token_eof(const struct tokenizer *tok)
{
    return tok->it >= tok->end;
}

char token_inc(struct tokenizer *tok)
{
    if (unlikely(token_eof(tok))) return 0;

    if (*tok->it == '\n') { tok->row++; tok->col = 0; }
    else { tok->col += (*tok->it == '\t' ? 8 : 1); } // fucking emacs...

    tok->it++;
    return *tok->it;
}


static void token_skip_spaces(struct tokenizer *tok)
{
    while (!token_eof(tok)) {

        if (likely(str_is_space(*tok->it))) {
            token_inc(tok);
            continue;
        }

        if (*tok->it == ';' && !tok->comments) {
            while (!token_eof(tok) && *tok->it != '\n')
                token_inc(tok);
            continue;
        }

        return;
    }
}

static void token_goto_space(struct tokenizer *tok)
{
    while (!token_eof(tok) && !str_is_space(*tok->it))
        token_inc(tok);
}


void token_goto_close(struct tokenizer *tok, const struct token *token)
{
    if (token && token->type == token_close) return;
    for (size_t depth = 1; depth && !token_eof(tok); token_inc(tok)) {
        token_skip_spaces(tok);
        if (*tok->it == '(') depth++;
        if (*tok->it == ')') depth--;
    }
}

struct token *token_next(struct tokenizer *tok, struct token *token)
{
    token_skip_spaces(tok);
    token->row = tok->row;
    token->col = tok->col;
    token->pos = tok->it - tok->base;

    if (token_eof(tok)) {
        token->type = token_nil;
        token->len = 0;
        return token;
    }

    switch (*tok->it) {
    case ';': { token->type = token_comment; break; }
    case '(': { token->type = token_open; break; }
    case ')': { token->type = token_close; break; }
    case '!': { token->type = token_atom; break; }
    case '?': { token->type = token_atom_make; break; }
    case '$': { token->type = token_reg; break; }
    case '.': { token->type = token_sep; break; }
    case '0'...'9': { token->type = token_number; break; }

    // We have an ambiguity between '-1' and '(- 1 1)' and there's no good way
    // to resolve it. Our hack is that '(-' always takes at least one argument
    // then we can guarantee a space after the symbol. Pretty shitty but oh well
    case '-': {
        const char *next = tok->it + 1;
        token->type = next < tok->end && str_is_space(*next) ?
            token_symbol : token_number;
        break;
    }

    default: {
        if (unlikely(!symbol_char(*tok->it))) {
            token_errf(tok, "invalid character for symbol: %c", *tok->it);
            token_goto_space(tok);
            token->type = token_nil;
            return token;
        }
        token->type = token_symbol; break;
    }
    }

    switch (token->type)
    {

    case token_comment:
    {
        assert(tok->comments);
        const char *first = tok->it;

        while (!token_eof(tok) && *tok->it != '\n')
            token_inc(tok);

        token->len = tok->it - first;
        break;
    }

    case token_open:
    case token_close:
    case token_sep: { token->len = 1; token_inc(tok); break; }

    case token_atom:
    case token_atom_make:
    case token_symbol:
    {
        const char *first = tok->it;
        if (token->type != token_symbol) { token_inc(tok); first++; }

        while(!token_eof(tok) && symbol_char(*tok->it)) token_inc(tok);

        token->len = tok->it - first;
        if (unlikely(token->len > symbol_cap))
            token_errf(tok, "symbol is too long: %u > %u", token->len, symbol_cap);

        token->value.s = make_symbol_len(first, legion_min(token->len, symbol_cap));

        // This suck but without it index is undershot by one due to the ! that
        // we skip at the start...
        if (token->type != token_symbol) token->len++;

        break;
    }

    case token_number:
    {
        const char *first = tok->it;
        while (!token_eof(tok) && str_is_number(*tok->it)) token_inc(tok);
        token->len = tok->it - first;

        size_t read = 0;
        if (token->len > 2 && first[0] == '0' && first[1] == 'x') {
            uint64_t value = 0;
            read = str_atox(first+2, token->len-2, &value) + 2;
            token->value.w = value;
        }
        else read = str_atod(first, token->len, &token->value.w);

        if (unlikely(read != token->len))
            token_errf(tok, "number was truncated: %u != %zu", token->len, read);

        break;
    }

    case token_reg:
    {
        if (unlikely(token_eof(tok))) {
            token_errf(tok, "invalid register value: %s", "eof");
            token->type = token_nil;
            token->len = 1;
            break;
        }


        const char *first = tok->it;
        char c = token_inc(tok);
        token_inc(tok);

        if (unlikely(c < '0' || c > '3')) {
            token_errf(tok, "invalid register value: %c", c);
            c = '0';
        }

        token->len = tok->it - first;
        token->value.w = c - '0';
        break;
    }

    default: { break; }
    }

    /* dbgf("tok: type=%u:%s, pos=%u, rc=%u:%u, len=%u, val={w:%lx, s:%s}", */
    /*         token->type, token_type_str(token->type), */
    /*         token->pos, token->row, token->col, token->len, */
    /*         token->value.w, token->value.s.c); */

    return token;
}

struct token *token_peek(struct tokenizer *tok, struct token *token)
{
    struct tokenizer copy = *tok;
    return token_next(&copy, token);
}

struct token *token_assert(
        struct tokenizer *tok, struct token *token, enum token_type exp)
{
    if (likely(token->type == exp)) return token;

    token_errf(tok, "unexpected token: %s != %s",
            token_type_str(token->type), token_type_str(exp));
    return NULL;
}

struct token *token_expect(
        struct tokenizer *tok, struct token *token, enum token_type exp)
{
    return token_assert(tok, token_next(tok, token), exp);
}


// -----------------------------------------------------------------------------
// stderr
// -----------------------------------------------------------------------------

#include <unistd.h>
#include <stdarg.h>

struct token_ctx
{
    bool ok;
    const char *name;
    struct tokenizer *tok;
};

static void token_ctx_err(void *_ctx, const char *fmt, ...)
{
    struct token_ctx *ctx = _ctx;
    ctx->ok = false;

    char str[256] = {0};
    char *it = str;
    char *end = str + sizeof(str);

    it += snprintf(str, end - it, "%s:%zu:%zu: ",
            ctx->name, ctx->tok->row+1, ctx->tok->col+1);

    va_list args;
    va_start(args, fmt);
    it += vsnprintf(it, end - it, fmt, args);
    va_end(args);

    *it = '\n'; it++;

    ssize_t ret = write(2, str, it - str);
    assert(ret == (it - str));
}

struct token_ctx *token_init_stderr(
        struct tokenizer *tok, const char *name, const char *src, size_t len)
{
    struct token_ctx *ctx = mem_alloc_t(ctx);
    *ctx = (struct token_ctx) { .ok = true, .name = name, .tok = tok };

    token_init(tok, src, len, token_ctx_err, ctx);
    return ctx;
}

void token_ctx_free(struct token_ctx *ctx)
{
    mem_free(ctx);
}

bool token_ctx_ok(struct token_ctx *ctx)
{
    return ctx->ok;
}
