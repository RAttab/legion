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
    man_markup_nil       = 0,
    man_markup_comment   = '%',

    man_markup_title     = '@',
    man_markup_section   = '=',
    man_markup_topic     = '-',

    man_markup_link      = '/',
    man_markup_link_ui   = '\\',

    man_markup_list      = '>',
    man_markup_list_end  = '<',

    man_markup_newline   = 'n',
    man_markup_tab       = 't',

    man_markup_underline = '_',
    man_markup_bold      = '*',

    man_markup_code      = '`',
    man_markup_eval      = '$',
    man_markup_item      = '!',
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
    case man_markup_link_ui:   { return man_markup_link_ui; }

    case man_markup_list:      { return man_markup_list; }
    case man_markup_list_end:  { return man_markup_list_end; }

    case man_markup_newline:   { return man_markup_newline; }
    case man_markup_tab:       { return man_markup_tab; }

    case man_markup_underline: { return man_markup_underline; }
    case man_markup_bold:      { return man_markup_bold; }

    case man_markup_code:      { return man_markup_code; }
    case man_markup_eval:      { return man_markup_eval; }
    case man_markup_item:      { return man_markup_item; }

    default:                   { return man_markup_nil; }
    }
}


struct man_parser
{
    bool ok;

    struct
    {
        const struct man_page *page;
        const char *it, *end;
        man_line line;
        uint8_t col;
    } in;

    struct
    {
        struct man *man;
        struct lisp *lisp;
        struct { uint8_t it, cap; } col;
        uint8_t indent;
        bool list;
    } out;
};

legion_printf(2, 3)
static void man_err(struct man_parser *parser, const char *fmt, ...)
{
    parser->ok = false;

    char str[1024] = {0};
    char *it = str, *end = it + sizeof(str);

    it += snprintf(it, end - it, "%s:%u:%u: ",
            parser->in.page->path,
            parser->in.line + 1,
            parser->in.col);

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

enum man_token_type : uint8_t
{
    man_token_nil = 0,
    man_token_eof,
    man_token_line,
    man_token_word,
    man_token_close,
    man_token_markup,
    man_token_paragraph,
};

const char *man_token_type_str(enum man_token_type type)
{
    switch (type)
    {
    case man_token_nil: { return "nil"; }
    case man_token_eof: { return "eof"; }
    case man_token_line: { return "line"; }
    case man_token_word: { return "word"; }
    case man_token_close: { return "close"; }
    case man_token_markup: { return "markup"; }
    case man_token_paragraph: { return "para"; }
    default: { assert(false); }
    }
}

struct legion_packed man_token
{
    enum man_token_type type;
    legion_pad(3);
    uint32_t len;
    const char *it;
};

static_assert(sizeof(struct man_token) == 16);

static struct man_token make_man_token(
        enum man_token_type type, const char *it, const char *end)
{
    return (struct man_token) { .type = type, .it = it, .len = end - it };
}

static struct man_token make_man_token_eof(void)
{
    return (struct man_token) { .type = man_token_eof };
}

static enum man_markup_type man_token_markup_type(const struct man_token *token)
{
    assert(token->type == man_token_markup);
    assert(token->len == 2);
    return man_markup_type(*(token->it + 1));
}

static bool man_token_assert(
        struct man_parser *parser,
        const struct man_token *token,
        enum man_token_type exp)
{
    if (likely(token->type == exp)) return true;

    man_err(parser, "unexpected token '%d != %d': %.*s",
            token->type, exp, token->len, token->it);
    return false;
}


// -----------------------------------------------------------------------------
// parser
// -----------------------------------------------------------------------------

static void man_parser_inc(struct man_parser *parser)
{
    assert(parser->in.it < parser->in.end);

    if (likely(*parser->in.it != '\n')) parser->in.col++;
    else { parser->in.line++; parser->in.col = 0; }

    parser->in.it++;
}

static struct man_token man_parser_next(struct man_parser *parser)
{
    size_t eols = 0;
    const char *start = parser->in.it;
    for (; parser->in.it < parser->in.end; man_parser_inc(parser)) {
        if (!str_is_space(*parser->in.it)) break;
        if (*parser->in.it == '\n') eols++;
    }

    if (unlikely(parser->in.it == parser->in.end))
        return make_man_token_eof();

    if (unlikely(eols > 1))
        return make_man_token(man_token_paragraph, start, parser->in.it);

    start = parser->in.it;
    assert(!str_is_space(*start));

    switch (*parser->in.it)
    {

    case '{': {
        if (parser->in.it + 2 > parser->in.end) {
            man_err(parser, "unexpected eof");
            return make_man_token_eof();
        }
        man_parser_inc(parser);

        char c = *parser->in.it;
        if (!man_markup_type(c)) {
            man_err(parser, "invalid markup '%c'", c);
            return make_man_token_eof();
        }
        man_parser_inc(parser);

        return make_man_token(man_token_markup, start, parser->in.it);
    }

    case '}': {
        man_parser_inc(parser);
        return make_man_token(man_token_close, start, parser->in.it);
    }

    default: {
        for (; parser->in.it < parser->in.end; man_parser_inc(parser)) {
            if (str_is_space(*parser->in.it)) break;
            if (*parser->in.it == '{' || *parser->in.it == '}') break;
        }
        return make_man_token(man_token_word, start, parser->in.it);
    }

    }
}

static struct man_token man_parser_peek(struct man_parser *parser)
{
    struct man_parser copy = *parser;
    return man_parser_next(&copy);
}

static struct man_token man_parser_until_close(struct man_parser *parser)
{
    const char *start = parser->in.it;
    for (; parser->in.it < parser->in.end; man_parser_inc(parser)) {
        if (*parser->in.it == '}') break;
        if (*parser->in.it == '\n') {
            man_err(parser, "unexpected eol");
            return make_man_token(man_token_line, start, parser->in.it);
        }
    }

    if (parser->in.it == parser->in.end) {
        man_err(parser, "unexpected eof");
        return make_man_token_eof();
    }

    assert(*parser->in.it == '}');
    man_parser_inc(parser);
    return make_man_token(man_token_line, start, parser->in.it - 1);
}

static struct man_token man_parser_line(struct man_parser *parser)
{
    const char *start = parser->in.it;

    if (unlikely(start == parser->in.end)) {
        man_err(parser, "unexpected eof");
        return make_man_token_eof();
    }

    if (*parser->in.it == '}') {
        man_parser_inc(parser);
        return make_man_token(man_token_close, start, parser->in.it - 1);
    }

    for (; parser->in.it < parser->in.end; man_parser_inc(parser)) {
        if (*parser->in.it == '}') break;
        if (*parser->in.it == '\n') {
            man_parser_inc(parser);
            return make_man_token(man_token_line, start, parser->in.it - 1);
        }
    }

    return make_man_token(man_token_line, start, parser->in.it);
}
