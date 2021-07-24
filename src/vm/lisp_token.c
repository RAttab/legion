/* lisp_token.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c

// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

static void lisp_skip_spaces(struct lisp *lisp)
{
    while (!lisp_eof(lisp)) {

        if (likely(str_is_space(*lisp->in.it))) {
            lisp_in_inc(lisp);
            continue;
        }

        if (*lisp->in.it == ';') {
            while (!lisp_eof(lisp) && *lisp->in.it != '\n')
                lisp_in_inc(lisp);
            continue;
        }

        return;
    }
}

static void lisp_goto_space(struct lisp *lisp)
{
    while (!lisp_eof(lisp) && !str_is_space(*lisp->in.it))
        lisp_in_inc(lisp);
}

static struct token *lisp_next(struct lisp *lisp)
{
    struct token *token = &lisp->token;

    lisp_skip_spaces(lisp);
    if (lisp_eof(lisp)) {
        token->type = token_nil;
        token->len = 0;
        return token;
    }

    token->row = lisp->in.row;
    token->col = lisp->in.col;

    switch (*lisp->in.it) {
    case '(': { token->type = token_open; break; }
    case ')': { token->type = token_close; break; }
    case '!': { token->type = token_atom; break; }
    case '$': { token->type = token_reg; break; }
    case '0'...'9': { token->type = token_num; break; }

    // We have an ambiguity between '-1' and '(- 1 1)' and there's no good way
    // to resolve it. Our hack is that '(-' always takes at least one argument
    // then we can guarantee a space after the symbol. Pretty shitty but oh well
    case '-': {
        const char *next = lisp->in.it + 1;
        token->type = next < lisp->in.end && str_is_space(*next) ?
            token_symb : token_num;
        break;
    }

    default: {
        if (unlikely(!symbol_char(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for symbol: %c", *lisp->in.it);
            lisp_goto_space(lisp);
            token->type = token_nil;
            return token;
        }
        token->type = token_symb; break;
    }
    }

    switch (token->type)
    {

    case token_open:
    case token_close: { token->len = 1; lisp_in_inc(lisp); break; }

    case token_atom:
    case token_symb: {
        const char *first = lisp->in.it;
        if (token->type == token_atom) { lisp_in_inc(lisp); first++; }

        while(!lisp_eof(lisp) && symbol_char(*lisp->in.it)) lisp_in_inc(lisp);

        token->len = lisp->in.it - first;
        if (unlikely(token->len > symbol_cap)) {
            lisp_err(lisp, "symbol is too long: %u > %u", token->len, symbol_cap);
            token->len = symbol_cap;
        }
        token->val.symb = make_symbol_len(first, token->len);
        break;
    }

    case token_num: {
        const char *first = lisp->in.it;
        while (!lisp_eof(lisp) && str_is_number(*lisp->in.it)) lisp_in_inc(lisp);

        token->len = lisp->in.it - first;
        size_t read = 0;
        if (token->len > 2 && first[0] == '0' && first[1] == 'x') {
            uint64_t value = 0;
            read = str_atox(first+2, token->len-2, &value) + 2;
            token->val.num = value;
        }
        else read = str_atod(first, token->len, &token->val.num);

        if (unlikely(read != token->len))
            lisp_err(lisp, "number was truncated: %u != %zu", token->len, read);
        break;
    }

    case token_reg: {
        if (unlikely(lisp_eof(lisp))) {
            lisp_err(lisp, "invalid register value: %s", "eof");
            token->type = token_nil;
            token->len = 1;
            break;
        }

        token->len = 2;

        char c = lisp_in_inc(lisp);
        lisp_in_inc(lisp);

        if (unlikely(c < '0' || c > '3')) {
            lisp_err(lisp, "invalid register value: %c", c);
            c = '0';
        }

        token->val.num = c - '0';
        break;
    }

    default: { break; }
    }

    /* dbg("tok: type=%u:%s, pos=%u:%u, len=%u, val={w:%lx, s:%s}", */
    /*         token->type, token_type_str(token->type), */
    /*         token->row, token->col, token->len, */
    /*         token->val.num, token->val.symb.c); */

    return token;
}

static struct token *lisp_assert_token(
        struct lisp *lisp, struct token *token, enum token_type exp)
{
    if (likely(token->type == exp)) return token;

    lisp_err(lisp, "unexpected token: %s != %s",
            token_type_str(token->type), token_type_str(exp));
    return NULL;
}


static struct token *lisp_expect(struct lisp *lisp, enum token_type exp)
{
    return lisp_assert_token(lisp, lisp_next(lisp), exp);
}

static void lisp_assert_close(struct lisp *lisp, struct token *token)
{
    if (!lisp_assert_token(lisp, token, token_close))
        lisp_goto_close(lisp);
    lisp->depth--;
}

static void lisp_expect_close(struct lisp *lisp)
{
    lisp_assert_close(lisp, lisp_next(lisp));
}
