/* man_render.c
   Rémi Attab (remi.attab@gmail.com), 23 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/

// included in man.c

// -----------------------------------------------------------------------------
// indent
// -----------------------------------------------------------------------------

enum
{
    man_indent_section_abs = 4,
    man_indent_topic_abs = 8,

    man_indent_list_rel = 8,
    man_indent_code_rel = 0,
};


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static struct markup *man_render_newline(struct man_parser *parser)
{
    parser->cols.curr = 0;
    return man_newline(parser->man);
}

static void man_render_repeat(
        struct man_parser *parser, struct markup *markup, char c, size_t len)
{
    parser->cols.curr += len;
    markup_repeat(markup, c, len);
}

static void man_render_str(
        struct man_parser *parser, struct markup *markup,
        const char *str, size_t len)
{
    parser->cols.curr += len;
    markup_str(markup, str, len);
}

static bool man_render_is_line_start(struct man_parser *parser)
{
    return man_current(parser->man)->type == markup_eol;
}

static void man_render_append(
        struct man_parser *parser,
        const struct man_token *token)
{
    size_t spaces = 1;
    size_t len = token->end - token->it;
    struct markup *markup = man_current(parser->man);

    if (parser->cols.curr + spaces + len > parser->cols.cap) {
        // \todo line justitication should happen here.

        (void) man_render_newline(parser);
        struct markup *new = man_markup(parser->man, markup_nil);
        markup_continue(new, markup);
        markup = new;
    }

    // \todo this is really bad but can't really think of a better solution at
    // the moment.
    struct markup *prev = man_previous(parser->man);
    if (!markup->len) {
        if (prev->type == markup_eol)
            spaces = parser->indent;
        if (prev->len && prev->text[prev->len - 1] == '(')
            spaces = 0;
        if (*token->it == ')')
            spaces = 0;
    }

    if (!markup->type) markup->type = markup_text;
    man_render_repeat(parser, markup, ' ', spaces);
    man_render_str(parser, markup, token->it, len);
}

static void man_render_word(
        struct man_parser *parser,
        const struct man_token *token)
{
    (void) man_markup(parser->man, markup_text);
    man_render_append(parser, token);
}

static void man_render_paragraph(struct man_parser *parser)
{
    if (man_current(parser->man)->type != markup_eol)
        (void) man_render_newline(parser);
    (void) man_render_newline(parser);
}

static void man_render_comment(struct man_parser *parser)
{
    struct man_token token = {0};
    while ((token = man_parser_next(parser)).type != man_token_eof) {
        if (token.type == man_token_close) return;
    }
    man_err(parser, "unexpected eof");
}

static void man_render_title(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);
    const size_t len = token.end - token.it;

    char title[len];
    memcpy(title, token.it, len);
    for (size_t i = 0; i < len; ++i) title[i] = toupper(title[i]);

    struct markup *markup = man_markup(parser->man, markup_text);
    if (parser->man->text.len > 1)
        man_err(parser, "title markup must be at the top");

    man_render_str(parser, markup, title, len);

    size_t center = ((parser->cols.cap / 2) - (sizeof(man_title) / 2));
    assert(len + 1 < center);
    man_render_repeat(parser, markup, ' ', center - markup->len);
    man_render_str(parser, markup, man_title, sizeof(man_title));

    size_t end = parser->cols.cap - len;
    assert(center + sizeof(man_title) + 1 < end);
    man_render_repeat(parser, markup, ' ', end - markup->len);
    man_render_str(parser, markup, title, len);

    (void) man_render_newline(parser);
    parser->indent = man_indent_section_abs;
}

static void man_render_section(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);
    if (!man_render_is_line_start(parser))
        man_err(parser, "section must be at the start of a new line");
    man_mark_section(parser->man);

    struct markup *markup = man_markup(parser->man, markup_bold);
    man_render_str(parser, markup, token.it, token.end - token.it);

    (void) man_render_newline(parser);
    parser->indent = man_indent_section_abs;
    parser->list = false;
}

static void man_render_topic(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);
    if (!man_render_is_line_start(parser))
        man_err(parser, "topic must be at the start of a new line");
    man_mark_section(parser->man);

    struct markup *markup = man_markup(parser->man, markup_bold);
    man_render_repeat(parser, markup, ' ', man_indent_section_abs);
    man_render_str(parser, markup, token.it, token.end - token.it);

    (void) man_render_newline(parser);
    parser->indent = man_indent_topic_abs;
    parser->list = false;
}

static void man_render_link(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);

    token.it--;
    assert(*token.it == man_markup_link);
    size_t len = token.end - token.it;

    struct link link = man_link(token.it, len);
    if (link_is_nil(link)) {
        man_err(parser, "unknown link '%.*s'", (unsigned) len, token.it);
        return;
    }

    struct markup *markup = man_markup(parser->man, markup_link);
    markup->link = link;
    man_render_append(parser, &token);

    (void) man_markup(parser->man, markup_nil);
}

static void man_render_list(struct man_parser *parser)
{
    if (parser->list) parser->indent -= man_indent_list_rel;
    parser->list = true;

    struct man_token token = man_parser_until_close(parser);
    size_t len = token.end - token.it;

    if (!man_render_is_line_start(parser))
        man_err(parser, "list must be at the start of a new line");

    struct markup *markup = man_markup(parser->man, markup_bold);
    man_render_repeat(parser, markup, ' ', parser->indent);
    man_render_str(parser, markup, token.it, len);

    size_t col = markup->len;
    markup = man_markup(parser->man, markup_nil);
    parser->indent += man_indent_list_rel;

    // The +1 in the if is to account for the space needed between the list
    // header and the text and the -1 in man_render_repeat is to account for the
    // space that is automatically added by man_render_append.
    if (col + 1 <= parser->indent)
        man_render_repeat(parser, markup, ' ', parser->indent - col - 1);
    else (void) man_render_newline(parser);
}

static void man_render_list_end(struct man_parser *parser)
{
    (void) man_parser_until_close(parser);

    if (!parser->list) { man_err(parser, "unmatched list end");  return; }
    parser->indent -= man_indent_list_rel;
    parser->list = false;
}

static void man_render_style(struct man_parser *parser, enum man_markup_type style)
{
    switch (style) {
    case man_markup_bold: { man_markup(parser->man, markup_bold); break; }
    case man_markup_underline: { man_markup(parser->man, markup_underline); break; }
    default: { assert(false); }
    }

    struct man_token token = {0};
    while ((token = man_parser_next(parser)).type != man_token_close) {
        if (!man_token_assert(parser, &token, man_token_word)) return;
        man_render_append(parser, &token);
    }

    (void) man_markup(parser->man, markup_nil);
}

static void man_render_code(struct man_parser *parser)
{
    if (!man_render_is_line_start(parser))
        man_err(parser, "code must be at the start of a new line");

    bool prologue = true;
    const size_t indent = parser->indent + man_indent_code_rel;

    struct man_token token = {0};
    while ((token = man_parser_line(parser)).type == man_token_line) {
        size_t len = token.end - token.it;
        if (len + indent > parser->cols.cap) {
            man_err(parser, "code line too long: %zu + %zu > %u",
                    indent, len, parser->cols.cap);
        }

        if (!len && prologue) continue;
        else prologue = true;

        struct markup *markup = man_markup(parser->man, markup_code);
        man_render_repeat(parser, markup, ' ', indent);
        man_render_str(parser, markup, token.it, len);
        (void) man_render_newline(parser);
    }
}

static void man_render_eval(struct man_parser *parser, struct lisp *lisp)
{
    struct man_token token = man_parser_until_close(parser);

    const char *start = token.it - 1;
    assert(*start == man_markup_eval);
    size_t len = token.end - start;

    struct lisp_ret ret = lisp_eval_const(lisp, start, len);
    if (!ret.ok) {
        man_err(parser, "invalid lisp '%.*s'", (unsigned) len, start);
        return;
    }

    char str[16] = {0};
    str_utox(ret.value, str, sizeof(str));
    token = make_man_token(man_token_word, str, str + sizeof(str));

    (void) man_markup(parser->man, markup_code);
    man_render_append(parser, &token);
    (void) man_markup(parser->man, markup_nil);
}

static struct man *man_page_render(
        const struct man_page *page, uint8_t cols, struct lisp *lisp)
{
    struct man_parser parser = {
        .ok = true,
        .man = man_new(),
        .page = page,
        .cols = { .cap = cols },
        .it = page->file.ptr,
        .end = page->file.ptr + page->file.len
    };

    struct man_token token = {0};
    while ((token = man_parser_next(&parser)).type != man_token_eof) {

        switch (token.type)
        {

        case man_token_word: { man_render_word(&parser, &token); break; }
        case man_token_paragraph: { man_render_paragraph(&parser); break; }

        case man_token_markup: {

            enum man_markup_type type = man_token_markup_type(&token);
            switch (type)
            {
            case man_markup_comment: { man_render_comment(&parser); break; }

            case man_markup_title: { man_render_title(&parser); break; }
            case man_markup_section: { man_render_section(&parser); break; }
            case man_markup_topic: { man_render_topic(&parser); break; }

            case man_markup_link: { man_render_link(&parser); break; }
            case man_markup_list : { man_render_list(&parser); break; }
            case man_markup_list_end: { man_render_list_end(&parser); break; }

            case man_markup_bold:
            case man_markup_underline: { man_render_style(&parser, type); break; }

            case man_markup_code: { man_render_code(&parser); break; }
            case man_markup_eval: { man_render_eval(&parser, lisp); break; }

            default: { man_err(&parser, "unknown markup"); break; }
            }

            break;
        }

        default: { man_err(&parser, "unexpected token '%u'", token.type); break; }
        }
    }

    if (!parser.ok) { man_free(parser.man); return NULL; }
    return parser.man;
}
