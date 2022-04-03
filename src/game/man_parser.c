/* man_parser.c
   Rémi Attab (remi.attab@gmail.com), 11 Feb 2022
   FreeBSD-style copyright and disclaimer apply
*/

// included in man.c


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

static const char man_title[] = "Legion's Programmer Manual";

enum man_markup_type
{
    man_markup_nil = 0,
    man_markup_comment   = '%',

    man_markup_title     = '@',
    man_markup_section   = '!',
    man_markup_topic     = '?',

    man_markup_link      = '/',
    man_markup_list      = '>',
    man_markup_list_end  = '<',

    man_markup_underline = '_',
    man_markup_bold      = '*',

    man_markup_code      = '`',
    man_markup_eval      = '(',
};

static enum man_markup_type man_markup_type(char c)
{
    switch (c)
    {
    case man_markup_comment:   { return man_markup_comment; }

    case man_markup_title:     { return man_markup_title; }
    case man_markup_section:   { return man_markup_section; }
    case man_markup_topic:     { return man_markup_topic; }

    case man_markup_link:      { return man_markup_link; }
    case man_markup_list:      { return man_markup_list; }
    case man_markup_list_end:  { return man_markup_list_end; }

    case man_markup_underline: { return man_markup_underline; }
    case man_markup_bold:      { return man_markup_bold; }

    case man_markup_code:      { return man_markup_code; }
    case man_markup_eval:      { return man_markup_eval; }
    default:                   { return man_markup_nil; }
    }
}


struct man_parser
{
    bool ok;

    struct man *man;
    const struct man_page *page;

    bool list;
    uint8_t indent;
    line_t line;
    struct { uint8_t curr, cap; } cols;

    const char *it, *end;
};

legion_printf(2, 3)
static void man_err(struct man_parser *parser, const char *fmt, ...)
{
    parser->ok = false;

    char str[1024] = {0};
    char *it = str, *end = it + sizeof(str);

    it += snprintf(it, end - it, "%s:%u: ",
            parser->page->path,
            parser->line + 1);

    va_list args;
    va_start(args, fmt);
    it += vsnprintf(it, end - it, fmt, args);
    va_end(args);

    assert(it + 1 < end);
    *it = '\n';
    it++;

    ssize_t ret = write(2, str, it - str);
    assert(ret == (it - str));
}


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

enum man_token_type
{
    man_token_nil = 0,
    man_token_eof,
    man_token_line,
    man_token_word,
    man_token_close,
    man_token_markup,
    man_token_paragraph,
};

struct man_token
{
    enum man_token_type type;
    const char *it, *end;
};

static struct man_token make_man_token(
        enum man_token_type type, const char *it, const char *end)
{
    return (struct man_token) { .type = type, .it = it, .end = end };
}

static struct man_token make_man_token_eof(void)
{
    return (struct man_token) { .type = man_token_eof };
}

static enum man_markup_type man_token_markup_type(const struct man_token *token)
{
    assert(token->type == man_token_markup);
    assert(token->end - token->it == 2);
    return man_markup_type(*(token->it + 1));
}

static bool man_token_assert(
        struct man_parser *parser,
        const struct man_token *token,
        enum man_token_type exp)
{
    if (likely(token->type == exp)) return true;

    man_err(parser, "unexpected token '%d != %d': %.*s",
            token->type, exp, (unsigned) (token->end - token->it), token->it);
    return false;
}


// -----------------------------------------------------------------------------
// parser
// -----------------------------------------------------------------------------

static void man_parser_inc(struct man_parser *parser)
{
    assert(parser->it < parser->end);

    if (*parser->it == '\n') parser->line++;
    parser->it++;
}

static struct man_token man_parser_next(struct man_parser *parser)
{
    size_t eols = 0;
    const char *start = parser->it;
    for (; parser->it < parser->end; man_parser_inc(parser)) {
        if (!str_is_space(*parser->it)) break;
        if (*parser->it == '\n') eols++;
    }

    if (unlikely(parser->it == parser->end))
        return make_man_token_eof();

    if (unlikely(eols > 1))
        return make_man_token(man_token_paragraph, start, parser->it);

    start = parser->it;
    assert(!str_is_space(*start));

    switch (*parser->it)
    {

    case '{': {
        parser->it += 2;
        if (parser->it > parser->end) {
            man_err(parser, "unexpected eof");
            return make_man_token_eof();
        }

        char c = *(parser->it - 1);
        if (!man_markup_type(c)) {
            man_err(parser, "invalid markup '%c'", c);
            return make_man_token_eof();
        }

        return make_man_token(man_token_markup, start, parser->it);
    }

    case '}': {
        man_parser_inc(parser);
        return make_man_token(man_token_close, start, parser->it);
    }

    default: {
        for (; parser->it < parser->end; ++parser->it) {
            if (str_is_space(*parser->it)) break;
            if (*parser->it == '{' || *parser->it == '}') break;
        }
        return make_man_token(man_token_word, start, parser->it);
    }

    }
}

static struct man_token man_parser_until_close(struct man_parser *parser)
{
    const char *start = parser->it;
    for (; parser->it < parser->end; man_parser_inc(parser)) {
        if (*parser->it == '}') break;
        if (*parser->it == '\n') {
            man_err(parser, "unexpected eol");
            return make_man_token(man_token_line, start, parser->it);
        }
    }

    if (parser->it == parser->end) {
        man_err(parser, "unexpected eof");
        return make_man_token_eof();
    }

    assert(*parser->it == '}');
    man_parser_inc(parser);
    return make_man_token(man_token_line, start, parser->it - 1);
}

static struct man_token man_parser_line(struct man_parser *parser)
{
    const char *start = parser->it;

    if (unlikely(start == parser->end)) {
        man_err(parser, "unexpected eof");
        return make_man_token_eof();
    }

    if (*parser->it == '}') {
        parser->cols.curr++;
        parser->it++;
        return make_man_token(man_token_close, start, parser->it);
    }

    for (; parser->it < parser->end; man_parser_inc(parser)) {
        if (*parser->it == '}') break;
        if (*parser->it == '\n') {
            parser->it++;
            return make_man_token(man_token_line, start, parser->it - 1);
        }
    }

    return make_man_token(man_token_line, start, parser->it);
}
