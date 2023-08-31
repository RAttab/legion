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
    man_indent_topic_header_abs = 2,
    man_indent_topic_body_abs = 4,

    man_indent_list_rel = 8,
    man_indent_code_rel = 0,
};


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static struct markup *man_render_newline(struct man_parser *parser)
{
    parser->out.col.it = 0;
    return man_newline(parser->out.man);
}

static void man_render_repeat(
        struct man_parser *parser, struct markup *markup, char c, size_t len)
{
    const char *start = man_text_repeat(parser->out.man, c, len);
    assert(start == markup->text + markup->len);

    markup->len += len;
    parser->out.col.it += len;
}

static void man_render_str(
        struct man_parser *parser, struct markup *markup,
        const char *str, size_t len)
{
    const char *start = man_text_str(parser->out.man, str, len);
    assert(start == markup->text + markup->len);

    markup->len += len;
    parser->out.col.it += len;
}

static bool man_render_is_line_start(struct man_parser *parser)
{
    return man_current(parser->out.man)->type == markup_eol;
}

static void man_render_append(
        struct man_parser *parser,
        const struct man_token *token)
{
    size_t spaces = 1;
    {
        switch (man_text_previous(parser->out.man)) {
        case '\n': { spaces = parser->out.indent; break; }
        case '(': case '[': { spaces = 0; break; }
        }

        switch (*token->it) {
        case ',': case '.':
        case ')': case ']': { spaces = 0; break; }
        }
    }

    struct markup *markup = man_current(parser->out.man);

    if (parser->out.col.it + spaces + token->len > parser->out.col.cap) {
        enum markup_type type = markup->type;
        struct link link = markup->link;

        (void) man_render_newline(parser);

        markup = man_markup(parser->out.man, type);
        markup->link = link;

        spaces = parser->out.indent;
    }

    if (!markup->type) markup->type = markup_text;
    if (spaces) man_render_repeat(parser, markup, ' ', spaces);
    man_render_str(parser, markup, token->it, token->len);
}

static void man_render_word(
        struct man_parser *parser,
        const struct man_token *token)
{
    (void) man_markup(parser->out.man, markup_text);
    man_render_append(parser, token);
}

static void man_render_paragraph(struct man_parser *parser)
{
    if (man_current(parser->out.man)->type != markup_eol)
        (void) man_render_newline(parser);

    if (man_previous(parser->out.man)->type != markup_eol)
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

    char title[token.len];
    memcpy(title, token.it, token.len);
    for (size_t i = 0; i < token.len; ++i) title[i] = toupper(title[i]);

    struct markup *markup = man_markup(parser->out.man, markup_text);
    if (parser->out.man->markup.len > 1)
        man_err(parser, "title markup must be at the top");

    man_render_str(parser, markup, title, token.len);

    size_t center = ((parser->out.col.cap / 2) - (sizeof(man_title) / 2));
    assert(token.len + 1 < center);
    man_render_repeat(parser, markup, ' ', center - markup->len);
    man_render_str(parser, markup, man_title, sizeof(man_title));

    size_t end = parser->out.col.cap - token.len;
    assert(center + sizeof(man_title) + 1 < end);
    man_render_repeat(parser, markup, ' ', end - markup->len);
    man_render_str(parser, markup, title, token.len);

    (void) man_render_newline(parser);
    parser->out.indent = man_indent_section_abs;
}

static void man_render_section(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);
    if (!man_render_is_line_start(parser))
        man_err(parser, "section must be at the start of a new line");
    man_mark_section(parser->out.man);

    struct markup *markup = man_markup(parser->out.man, markup_bold);
    man_render_str(parser, markup, token.it, token.len);

    (void) man_render_newline(parser);
    parser->out.indent = man_indent_section_abs;
    parser->out.list = false;

    while (man_parser_peek(parser).type == man_token_paragraph)
        man_parser_next(parser);
}

static void man_render_topic(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);
    if (!man_render_is_line_start(parser))
        man_err(parser, "topic must be at the start of a new line");
    man_mark_section(parser->out.man);

    struct markup *markup = man_markup(parser->out.man, markup_bold);
    man_render_repeat(parser, markup, ' ', man_indent_topic_header_abs);
    man_render_str(parser, markup, token.it, token.len);

    (void) man_render_newline(parser);
    parser->out.indent = man_indent_topic_body_abs;
    parser->out.list = false;

    while (man_parser_peek(parser).type == man_token_paragraph)
        man_parser_next(parser);
}

static void man_render_link(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);

    // Links starts with / which is currently in the previous markup token which
    // is annoying so we cheat a little and expand our current token to include
    // the /.
    token.it--;
    token.len++;
    assert(*token.it == man_markup_link);

    struct link link = man_link(token.it, token.len);
    if (link_is_nil(link)) {
        man_err(parser, "unknown link '%.*s'", token.len, token.it);
        return;
    }

    struct markup *markup = man_markup(parser->out.man, markup_link);
    markup->link = link;
    man_render_append(parser, &token);

    (void) man_markup(parser->out.man, markup_nil);
}

static void man_render_link_ui(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);

    const char *it = token.it;
    const char *end = token.it + token.len;
    for (; it < end && *it != '\\'; it++);

    man_page page = 0;
    if (!strncmp(token.it, "tape", it - token.it)) page = link_ui_tape;
    else {
        man_err(parser, "unknown ui link prefix: %.*s",
                (unsigned) (token.it - it), token.it);
        return;
    }

    const char *start = it + 1;
    if ((uint64_t) (end - start) > symbol_cap) {
        man_err(parser, "ui link suffix too long to be an atom: %.*s",
                (unsigned) (end - start), start);
        return;
    }

    const size_t len = 1 + (end - start);
    char lisp[1 + symbol_cap] = { [0] = '!' };
    memcpy(lisp + 1, start, end - start);

    struct lisp_ret ret = lisp_eval_const(parser->out.lisp, lisp, len);
    if (!ret.ok || ret.value > UINT16_MAX) {
        man_err(parser, "invalid link suffix '%s' -> %lx", lisp, ret.value);
        return;
    }

    // Links starts with \ which is currently in the previous markup token which
    // is annoying so we cheat a little and expand our current token to include
    // the \.
    token.it--;
    token.len++;
    assert(*token.it == man_markup_link_ui);

    struct markup *markup = man_markup(parser->out.man, markup_link);
    markup->link = make_link(page, ret.value);
    man_render_append(parser, &token);

    (void) man_markup(parser->out.man, markup_nil);
}

static void man_render_list(struct man_parser *parser)
{
    if (parser->out.list) parser->out.indent -= man_indent_list_rel;
    parser->out.list = true;

    if (!man_render_is_line_start(parser))
        man_err(parser, "list must be at the start of a new line");

    struct man_token token = man_parser_until_close(parser);

    struct markup *markup = man_markup(parser->out.man, markup_bold);
    man_render_repeat(parser, markup, ' ', parser->out.indent);
    man_render_str(parser, markup, token.it, token.len);

    size_t col = markup->len;
    markup = man_markup(parser->out.man, markup_nil);
    parser->out.indent += man_indent_list_rel;

    // The +1 in the if is to account for the space needed between the list
    // header and the text and the -1 in man_render_repeat is to account for the
    // space that is automatically added by man_render_append.
    if (col + 1 <= parser->out.indent)
        man_render_repeat(parser, markup, ' ', parser->out.indent - col - 1);
    else (void) man_render_newline(parser);
}

static void man_render_new_line(struct man_parser *parser)
{
    (void) man_parser_until_close(parser);
    (void) man_render_newline(parser);
}

static void man_render_tab(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);

    uint64_t len = 0;
    (void) str_atou(token.it, token.len, &len);

    size_t col = len + parser->out.indent;
    if (col < parser->out.col.it) return;

    size_t spaces = col - parser->out.col.it;
    man_render_repeat(parser, man_markup(parser->out.man, markup_text), ' ', spaces);
}

static void man_render_list_end(struct man_parser *parser)
{
    (void) man_parser_until_close(parser);

    if (!parser->out.list) { man_err(parser, "unmatched list end");  return; }
    parser->out.indent -= man_indent_list_rel;
    parser->out.list = false;
}

static void man_render_style(struct man_parser *parser, enum man_markup_type style)
{
    switch (style) {
    case man_markup_bold: { man_markup(parser->out.man, markup_bold); break; }
    case man_markup_underline: { man_markup(parser->out.man, markup_underline); break; }
    default: { assert(false); }
    }

    struct man_token token = {0};
    while ((token = man_parser_next(parser)).type != man_token_close) {
        if (!man_token_assert(parser, &token, man_token_word)) return;
        man_render_append(parser, &token);
    }

    (void) man_markup(parser->out.man, markup_nil);
}

static void man_render_code(struct man_parser *parser)
{
    if (!man_render_is_line_start(parser))
        man_err(parser, "code must be at the start of a new line");

    bool prologue = true;
    const size_t indent = parser->out.indent + man_indent_code_rel;

    struct man_token token = {0};
    while ((token = man_parser_line(parser)).type == man_token_line) {
        if (token.len + indent > parser->out.col.cap) {
            man_err(parser, "code line too long: %zu + %u > %u",
                    indent, token.len, parser->out.col.cap);
        }

        if (!token.len && prologue) continue;
        else prologue = false;

        struct markup *markup = man_markup(parser->out.man, markup_text);
        man_render_repeat(parser, markup, ' ', indent);

        markup = man_markup(parser->out.man, markup_code);
        man_render_str(parser, markup, token.it, token.len);

        (void) man_render_newline(parser);
    }
}

static void man_render_eval(struct man_parser *parser)
{
    struct man_token token = man_parser_until_close(parser);

    struct lisp_ret ret = lisp_eval_const(parser->out.lisp, token.it, token.len);
    if (!ret.ok) {
        man_err(parser, "invalid lisp '%.*s'", token.len, token.it);
        return;
    }

    char str[16] = {0};
    str_utox(ret.value, str, sizeof(str));
    token = make_man_token(man_token_word, str, str + sizeof(str));

    (void) man_markup(parser->out.man, markup_underline);
    man_render_append(parser, &token);
    (void) man_markup(parser->out.man, markup_nil);
}

static struct man *man_page_render(
        const struct man_page *page, uint8_t cols, struct lisp *lisp)
{
    struct man_parser parser = {
        .ok = true,

        .in = {
            .page = page,
            .it = page->data,
            .end = page->data + page->data_len,
            .line = 0, .col = 0,
        },

        .out = {
            .man = man_new(page->data_len * 2),
            .lisp = lisp,
            .col = { .it = 0, .cap = cols },
            .indent = 0,
            .list = false,
        },
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

            case man_markup_title:   { man_render_title(&parser); break; }
            case man_markup_section: { man_render_section(&parser); break; }
            case man_markup_topic:   { man_render_topic(&parser); break; }

            case man_markup_link:      { man_render_link(&parser); break; }
            case man_markup_link_ui:   { man_render_link_ui(&parser); break; }

            case man_markup_list :    { man_render_list(&parser); break; }
            case man_markup_list_end: { man_render_list_end(&parser); break; }

            case man_markup_newline: { man_render_new_line(&parser); break; }
            case man_markup_tab: { man_render_tab(&parser); break; }

            case man_markup_bold:
            case man_markup_underline: { man_render_style(&parser, type); break; }

            case man_markup_code: { man_render_code(&parser); break; }
            case man_markup_eval: { man_render_eval(&parser); break; }
            case man_markup_item: { man_parser_until_close(&parser); break; }

            default: { man_err(&parser, "unknown markup"); break; }
            }

            break;
        }

        default: { man_err(&parser, "unexpected token '%u'", token.type); break; }
        }
    }

    if (!parser.ok) { man_free(parser.out.man); return NULL; }
    return parser.out.man;
}
