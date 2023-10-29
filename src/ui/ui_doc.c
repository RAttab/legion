/* ui_doc.c
   Rémi Attab (remi.attab@gmail.com), 11 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_doc_style_default(struct ui_style *s)
{
    s->doc = (struct ui_doc_style) {
        .text = { .font = font_base, .fg = s->rgba.fg, .bg = s->rgba.bg },
        .bold = { .font = font_bold, .fg = s->rgba.fg, .bg = s->rgba.bg },

        .code = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = rgba_gray_a(0x66, 0x33),

            .comment = s->rgba.code.comment,
            .keyword = s->rgba.code.keyword,
            .atom = s->rgba.code.atom,
        },

#define make_from(src) { .font = font_base, .fg = (src).fg, .bg = (src).bg }
        .link =    make_from(s->rgba.link.idle),
        .hover =   make_from(s->rgba.link.hover),
        .pressed = make_from(s->rgba.link.pressed),
#undef make_from

        .underline = { .fg = s->rgba.fg, .offset = 2 },

        .copy = {
            .margin = s->button.base.margin.w,
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = s->button.base.idle.bg,
            .hover = s->button.base.hover.bg,
            .pressed = s->button.base.pressed.bg,
        },
    };
}


// -----------------------------------------------------------------------------
// doc
// -----------------------------------------------------------------------------

struct ui_doc ui_doc_new(struct dim dim)
{
    const struct ui_doc_style *s = &ui_st.doc;
    struct dim cell = engine_cell();

    struct ui_doc ui = {
        .w = make_ui_widget(dim),
        .s = *s,
        .p = ui_panel_current(),

        .scroll = ui_scroll_new(dim, cell),
        .cols = (dim.w / cell.w) - 2,
    };

    return ui;
}

void ui_doc_free(struct ui_doc *doc)
{
    ui_scroll_free(&doc->scroll);
    if (doc->man) man_free(doc->man);
    if (doc->copy.cap) free(doc->copy.buffer);
}


void ui_doc_open(struct ui_doc *doc, struct link link, struct lisp *lisp)
{
    struct man *man = man_open(link.page, doc->cols, lisp);
    if (!man) { ui_log(st_error, "unknown man link"); return; }

    if (doc->man) man_free(doc->man);
    doc->man = man;

    ui_scroll_update_rows(&doc->scroll, man_lines(doc->man));
    doc->scroll.rows.first = man_section_line(doc->man, link.section);
}

static void ui_doc_copy(struct ui_doc *doc)
{
    assert(doc->copy.line);

    doc->copy.len = 0;
    const struct markup *it = man_line_markup(doc->man, doc->copy.line);

    for (; it; it = man_next_markup(doc->man, it)) {
        switch (it->type)
        {

        case markup_code: {
            while (doc->copy.len + it->len > doc->copy.cap) {
                size_t old = legion_xchg(&doc->copy.cap, doc->copy.cap ? doc->copy.cap * 2 : 128);
                doc->copy.buffer = realloc_zero(doc->copy.buffer, old, doc->copy.cap, 1);
            }

            memcpy(doc->copy.buffer + doc->copy.len, it->text, it->len);
            doc->copy.len += it->len;

            doc->copy.buffer[doc->copy.len] = '\n';
            doc->copy.len++;
            break;
        }

        case markup_code_end:  {
            ui_clipboard_copy(doc->copy.buffer, doc->copy.len);
            return;
        }

        default: { break; }
        }
    }

    assert(false);
}

void ui_doc_event(struct ui_doc *doc)
{
    if (!doc->man) return;

    ui_scroll_event(&doc->scroll);

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        if (ev->state != ev_state_down) continue;

        struct pos cursor = ev_mouse_pos();
        if (!rect_contains(doc->w, cursor)) continue;

        if (doc->copy.line && rect_contains(doc->copy.rect, cursor)) {
            ui_doc_copy(doc);
            continue;
        }

        struct dim cell = engine_cell();
        uint32_t col = (cursor.x - doc->w.x) / cell.w;
        man_line line = (cursor.y - doc->w.y) / cell.h;
        line += ui_scroll_first_row(&doc->scroll);

        struct link link = man_click(doc->man, line, col);
        if (link_is_nil(link)) continue;

        if (link.page == link_ui_tape)
            ui_tapes_show(link.section);
        else ui_man_show(link);

        ev_consume_button(ev);
    }

    for (auto ev = ev_next_key(nullptr); ev; ev = ev_next_key(ev)) {
        if (ui_focus_panel() != doc->p) continue;

        switch (ev->c)
        {
        case ev_key_up: { ui_scroll_move_rows(&doc->scroll, -1); break; }
        case ev_key_down: { ui_scroll_move_rows(&doc->scroll, 1); break; }

        case ev_key_page_up: { ui_scroll_page_up(&doc->scroll); break; }
        case ev_key_page_down: {ui_scroll_page_down(&doc->scroll); break; }

        case ev_key_home: { doc->scroll.rows.first = 0; break; }
        case ev_key_end: { doc->scroll.rows.first = doc->scroll.rows.total - 1; break; }

        default: { continue; }
        }

        ev_consume_key(ev);
    }
}

void ui_doc_render(struct ui_doc *doc, struct ui_layout *layout)
{
    if (!doc->man) return;

    struct ui_layout inner = ui_scroll_render(&doc->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;
    doc->w = doc->scroll.w;

    struct pos pos = inner.base.pos;
    struct dim cell = engine_cell();
    unit baseline = engine_cell_baseline();

    man_line line = ui_scroll_first_row(&doc->scroll);
    const man_line end = ui_scroll_last_row(&doc->scroll);
    const struct markup *it = man_line_markup(doc->man, line);

    struct { man_line line; unit y; bool pressed; } copy = {0};
    doc->copy.line = 0;

    enum : render_layer
    {
        layer_bg = 0,
        layer_text,
        layer_underline,
        layer_copy_bg,
        layer_copy_fg,
        layer_len,
    };
    const render_layer l = render_layer_push(layer_len);


    for (; it && line < end; it = man_next_markup(doc->man, it)) {
        switch (it->type)
        {

        case markup_nil: { break; }

        case markup_eol: {
            pos.x = inner.base.pos.x;
            pos.y += cell.h;
            line++;
            break;
        }

        case markup_text: {
            render_font_bg(
                    l + layer_text, doc->s.text.font,
                    doc->s.text.fg, doc->s.text.bg,
                    pos, it->text, it->len);
            pos.x += it->len * cell.w;
            break;
        }

        case markup_underline: {
            render_font_bg(
                    l + layer_text, doc->s.text.font,
                    doc->s.text.fg, doc->s.text.bg,
                    pos, it->text, it->len);

            size_t skip = 0;
            while (skip < it->len && it->text[skip] == ' ') skip++;

            unit line_y = pos.y + baseline + doc->s.underline.offset;
            unit line_start = pos.x + (skip * cell.w);
            unit line_end = pos.x + (it->len * cell.w);

            render_line(l + layer_underline, doc->s.underline.fg, (struct line) {
                        make_pos(line_start, line_y), make_pos(line_end, line_y) });

            pos.x += it->len * cell.w;
            break;
        }

        case markup_bold: {
            render_font_bg(
                    l + layer_text, doc->s.bold.font,
                    doc->s.bold.fg, doc->s.bold.bg,
                    pos, it->text, it->len);
            pos.x += it->len * cell.w;
            break;
        }

        case markup_code_begin: { copy.line = line; copy.y = pos.y; break; }

        case markup_code: {
            enum render_font font =doc->s.code.font;

            struct rect rect = {
                .x = pos.x,
                .y = pos.y,
                .w = inner.base.dim.w - (pos.x - inner.base.pos.x),
                .h = cell.h,
            };

            if (ev_mouse_in(rect)) {
                doc->copy.line = copy.line;
                doc->copy.rect.y = copy.y;
            }

            render_rect_fill(l + layer_bg, doc->s.code.bg, rect);

            struct ast_node node = {0};
            while (ast_step(it->text, it->len, &node)) {
                struct rgba fg =
                    node.type == ast_comment ? doc->s.code.comment :
                    node.type == ast_keyword ? doc->s.code.keyword :
                    node.type == ast_atom ? doc->s.code.atom :
                    doc->s.code.fg;

                render_font(
                        l + layer_text, font, fg,
                        make_pos(pos.x + (node.pos * cell.w), pos.y),
                        it->text + node.pos, node.len);
            }

            pos.x += it->len * cell.w;
            break;
        }

        case markup_code_end: { copy.line = 0; break; }

        case markup_link: {
            enum render_font font = doc->s.link.font;
            struct rgba fg = { 0 }, bg = { 0 };

            struct rect rect = {
                .x = pos.x,
                .y = pos.y,
                .w = cell.w * it->len,
                .h = cell.h,
            };

            if (!ev_mouse_in(rect)) {
                font = doc->s.link.font;
                fg = doc->s.link.fg;
                bg = doc->s.link.bg;
            }
            else if (ev_button_down(ev_button_left)) {
                font = doc->s.pressed.font;
                fg = doc->s.pressed.fg;
                bg = doc->s.pressed.bg;
            }
            else {
                font = doc->s.hover.font;
                fg = doc->s.hover.fg;
                bg = doc->s.hover.bg;
            }

            render_font_bg(l + layer_text, font, fg, bg, pos, it->text, it->len);
            pos.x += it->len * cell.w;
            break;
        }

        default: { assert(false); }
        }
    }

    if (doc->copy.line) {
        constexpr size_t chars = 4;
        enum render_font font = doc->s.copy.font;

        struct rect *rect = &doc->copy.rect;
        rect->w = (chars * cell.w) + (doc->s.copy.margin * 2);
        rect->x = (inner.base.pos.x + inner.base.dim.w) - rect->w;
        rect->h = cell.h;

        bool in = ev_mouse_in(*rect);
        bool down = ev_button_down(ev_button_left);

        struct rgba bg =
            (in && down) ? doc->s.copy.pressed :
            in ? doc->s.copy.hover :
            doc->s.copy.bg;


        render_rect_fill(l + layer_copy_bg, bg, *rect);
        render_font(
                l + layer_copy_fg,
                font,
                doc->s.copy.fg,
                make_pos(rect->x + doc->s.copy.margin, rect->y),
                "copy", chars);
    }

    render_layer_pop();
}
