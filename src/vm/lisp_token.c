/* lisp_token.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c

// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

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

static void lisp_skip_spaces(struct lisp *lisp)
{
    while (!lisp_eof(lisp) && lisp_is_space(*lisp->in.it)) lisp_in_inc(lisp);
}

static void lisp_goto_space(struct lisp *lisp)
{
    while (!lisp_is_space(*lisp->in.it)) lisp_in_inc(lisp);
}

static struct token *lisp_next(struct lisp *lisp)
{
    lisp_skip_spaces(lisp);
    if (lisp_eof(lisp)) return token_nil;

    lisp->token.row = lisp->in.row;
    lisp->token.col = lisp->in.col;

    switch (*lisp->in.it) {
    case '(': { lisp->token.type = token_open; break; }
    case ')': { lisp->token.type = token_close; break; }
    case '!': { lisp->token.type = token_atom; break; }
    case '$': { lisp->token.type = token_reg; break; }
    case '0'...'9': { lisp->token.type = token_num; break; }
    default: {
        if (unlikely(!lisp_is_symb(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for symbol: %c", *lisp->in.it);
            lisp_goto_space(lisp);
            lisp->token.type = token_nil;
            return;
        }
        lisp->token.type = token_symb; break;
    }
    }

    switch (lisp->token.type)
    {

    case token_atom:
    case token_symb: {
        char *first = lisp->in.it;
        while(!lisp_eof(lisp) && lisp_is_symb(*lisp->in.it)) lisp_in_inc(lisp);

        lisp->token.len = lisp->in.it - first;
        if (unlikely(lisp->token.len > symbol_cap)) {
            lisp_err(lisp, "symbol is too long: %zu > %u", lisp->token.len, symb_cap);
            len = symbol_cap;
        }
        lisp->token.val.symb = make_symbol_len(first, lisp->token.len);

        if (unlikely(!lisp_is_space(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for symbol: %c", *lisp->in.it);
            lisp_goto_space(lisp);
        }

        break;
    }

    case token_num: {
        char *first = lisp->in.it;
        while (!lisp_eof(lisp) && lisp_is_num(*lisp->in.it)) lisp_in_inc(lisp);

        lisp->token.len = lisp->in.it - first;
        size_t read = 0;
        if (lisp->token.len > 2 && first[0] == '0' && first[1] == 'x')
            read = str_atox(lisp->in.it+2, lisp->token.len-2, &lisp->token.val.num) + 2;
        else read = str_atou(lisp->in.it, lisp->token.len, &lisp->token.val.num);

        if (unlikely(read != lisp->token.len))
            lisp_err(lisp, "number was truncated: %zu != %zu", lisp->token.len, read);

        if (unlikely(!lisp_is_space(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for number: %c", *lisp->in.it);
            lisp_goto_space(lisp);
        }

        break;
    }

    case token_reg: {
        if (unlikely(lisp_eof(lisp))) {
            lisp_err(lisp, "invalid register value: %s", "eof");
            lisp->token.type = token_nil;
            lisp->token.len = 1;
            return;
        }

        lisp->token.len = 2;

        char c = lisp_in_inc(lisp);
        if (unlikely(c < '0' || c > '3')) {
            lisp_err(lisp, "invalid register value: %c", c);
            c = '0';
        }

        lisp->token.val.num = c - '1';

        if (unlikely(!lisp_is_space(*lisp->in.it))) {
            lisp_err(lisp, "invalid character for register: %c", *lisp->in.it);
            lisp_goto_space(lisp);
        }

        break;
    }

    default: { break; }
    }

    return &lisp->token;
}

static struct token *lisp_expect(struct lisp *lisp, enum token_type exp)
{
    if (likely(lisp_next(lisp)->type != exp)) return &lisp->token;

    lisp_err(lisp, "unpextected token: %s != %s",
            token_type_str(lisp->token.type), token_type_str(exp));
    return NULL;
}
