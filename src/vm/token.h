/* token.h
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "vm/symbol.h"


// -----------------------------------------------------------------------------
// token_type
// -----------------------------------------------------------------------------

enum token_type
{
    token_nil = 0,
    token_open,
    token_close,
    token_atom,
    token_symbol,
    token_number,
    token_reg,
};

const char *token_type_str(enum token_type type);


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

struct token
{
    enum token_type type;
    uint32_t row, col, len;
    union { struct symbol s; word_t w; } value;
};


// -----------------------------------------------------------------------------
// tokenizer
// -----------------------------------------------------------------------------

typedef void (*token_err_fn_t) (void *, const char *fmt, ...);

struct tokenizer
{
    size_t row, col;
    const char *base, *it, *end;

    void *err_ctx;
    token_err_fn_t err_fn;
};

void token_init(
        struct tokenizer *,
        const char *src, size_t len,
        token_err_fn_t fn, void *ctx);

bool token_eof(const struct tokenizer *);
char token_inc(struct tokenizer *);
void token_goto_close(struct tokenizer *);

struct token *token_next(struct tokenizer *, struct token *);
struct token *token_peek(struct tokenizer *, struct token *);
struct token *token_expect(struct tokenizer *, struct token *, enum token_type exp);
struct token *token_assert(struct tokenizer *, struct token *, enum token_type exp);