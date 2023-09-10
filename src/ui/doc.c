/* doc.c
   Rémi Attab (remi.attab@gmail.com), 11 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "doc.h"
#include "render/ui.h"

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_doc_style_default(struct ui_style *s)
{
    s->doc = (struct ui_doc_style) {
        .text = { .font = s->font.base, .fg = s->rgba.fg, .bg = s->rgba.bg },
        .bold = { .font = s->font.bold, .fg = s->rgba.fg, .bg = s->rgba.bg },

        .code = {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = rgba_gray_a(0x66, 0x33),

            .comment = s->rgba.code.comment,
            .keyword = s->rgba.code.keyword,
            .atom = s->rgba.code.atom,
        },

#define make_from(src) { .font = s->font.base, .fg = (src).fg, .bg = (src).bg }
        .link =    make_from(s->rgba.link.idle),
        .hover =   make_from(s->rgba.link.hover),
        .pressed = make_from(s->rgba.link.pressed),
#undef make_from

        .underline = { .fg = s->rgba.fg, .offset = 2 },

        .copy = {
            .margin = s->button.base.margin.w,
            .font = s->font.base,
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

static void ui_doc_check_font(const struct font *base, const struct font *other)
{
    assert(base->glyph_w == other->glyph_w);
    assert(base->glyph_h == other->glyph_h);
}

struct ui_doc ui_doc_new(struct dim dim)
{
    const struct ui_doc_style *s = &ui_st.doc;

    const struct font *font = s->text.font;
    ui_doc_check_font(font, s->bold.font);
    ui_doc_check_font(font, s->code.font);
    ui_doc_check_font(font, s->link.font);
    ui_doc_check_font(font, s->hover.font);
    ui_doc_check_font(font, s->pressed.font);

    struct ui_doc ui = {
        .w = ui_widget_new(dim.w, dim.h),
        .s = *s,
        .scroll = ui_scroll_new(dim, make_dim(font->glyph_w, font->glyph_h)),
        .cols = (dim.w / font->glyph_w) - 2,
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

static enum ui_ret ui_doc_copy(struct ui_doc *doc)
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
            return ui_consume;
        }

        default: { break; }
        }
    }

    assert(false);
}

enum ui_ret ui_doc_event(struct ui_doc *doc, const SDL_Event *ev)
{
    enum ui_ret ret = 0;
    if ((ret = ui_scroll_event(&doc->scroll, ev))) return ret;

    if (!doc->man) return ui_nil;

    switch (ev->type)
    {

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Rect rect = ui_widget_rect(&doc->w);
        if (!ui_cursor_in(&rect)) return ui_nil;
        doc->pressed = true;
        return ui_consume;
    }

    case SDL_MOUSEBUTTONUP: {
        doc->pressed = false;

        SDL_Point cursor = ui_cursor_point();
        SDL_Rect rect = ui_widget_rect(&doc->w);
        if (!SDL_PointInRect(&cursor, &rect)) return ui_nil;

        if (doc->copy.line && SDL_PointInRect(&cursor, &doc->copy.rect))
            return ui_doc_copy(doc);

        const struct font *font = doc->s.text.font;
        uint8_t col = (cursor.x - doc->w.pos.x) / font->glyph_w;
        man_line line = (cursor.y - doc->w.pos.y) / font->glyph_h;
        line += ui_scroll_first_row(&doc->scroll);

        struct link link = man_click(doc->man, line, col);
        if (link_is_nil(link)) return ui_nil;

        if (link.page == link_ui_tape)
            ui_tapes_show(link.section);
        else ui_man_show(link);

        return ui_consume;
    }

    case SDL_KEYDOWN: {
        switch (ev->key.keysym.sym)
        {
        case SDLK_UP: { ui_scroll_move_rows(&doc->scroll, -1); return ui_consume; }
        case SDLK_DOWN: { ui_scroll_move_rows(&doc->scroll, 1); return ui_consume; }

        case SDLK_PAGEUP: { ui_scroll_page_up(&doc->scroll); return ui_consume; }
        case SDLK_PAGEDOWN: {ui_scroll_page_down(&doc->scroll); return ui_consume; }

        case SDLK_HOME: { doc->scroll.rows.first = 0; return ui_consume; }
        case SDLK_END: {
            doc->scroll.rows.first = doc->scroll.rows.total - 1;
            return ui_consume;
        }

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }

}

void ui_doc_render(
        struct ui_doc *doc, struct ui_layout *layout, SDL_Renderer *renderer)
{
    if (!doc->man) return;

    struct ui_layout inner = ui_scroll_render(&doc->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;
    doc->w = doc->scroll.w;

    struct pos pos = inner.base.pos;
    man_line line = ui_scroll_first_row(&doc->scroll);
    const man_line end = ui_scroll_last_row(&doc->scroll);
    const struct markup *it = man_line_markup(doc->man, line);

    struct { man_line line; int16_t y; bool pressed; } copy = {0};
    doc->copy.line = 0;

    for (; it && line < end; it = man_next_markup(doc->man, it)) {
        switch (it->type)
        {

        case markup_nil: { break; }

        case markup_eol: {
            pos.x = inner.base.pos.x;
            pos.y += doc->s.text.font->glyph_h;
            line++;
            break;
        }

        case markup_text: {
            const struct font *font = doc->s.text.font;
            font_render_bg(
                    font,
                    renderer,
                    pos_as_point(pos),
                    doc->s.text.fg,
                    doc->s.text.bg,
                    it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_underline: {
            const struct font *font = doc->s.text.font;
            font_render_bg(
                    font,
                    renderer,
                    pos_as_point(pos),
                    doc->s.text.fg,
                    doc->s.text.bg,
                    it->text, it->len);

            size_t skip = 0;
            while (skip < it->len && it->text[skip] == ' ') skip++;

            int line_y = pos.y + font->glyph_baseline + doc->s.underline.offset;
            int line_start = pos.x + (skip * font->glyph_w);
            int line_end = pos.x + (it->len * font->glyph_w);

            rgba_render(doc->s.underline.fg, renderer);
            sdl_err(SDL_RenderDrawLine(renderer, line_start, line_y, line_end, line_y));

            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_bold: {
            const struct font *font = doc->s.bold.font;
            font_render_bg(
                    font,
                    renderer,
                    pos_as_point(pos),
                    doc->s.bold.fg,
                    doc->s.bold.bg,
                    it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_code_begin: { copy.line = line; copy.y = pos.y; break; }

        case markup_code: {
            const struct font *font = doc->s.code.font;

            SDL_Rect rect = {
                .x = pos.x,
                .y = pos.y,
                .w = inner.base.dim.w - (pos.x - inner.base.pos.x),
                .h = font->glyph_h,
            };

            if (ui_cursor_in(&rect)) {
                doc->copy.line = copy.line;
                doc->copy.rect.y = copy.y;
            }

            rgba_render(doc->s.code.bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &rect));

            struct ast_node node = {0};
            while (ast_step(it->text, it->len, &node)) {
                struct rgba fg =
                    node.type == ast_comment ? doc->s.code.comment :
                    node.type == ast_keyword ? doc->s.code.keyword :
                    node.type == ast_atom ? doc->s.code.atom :
                    doc->s.code.fg;

                SDL_Point pt = {
                    .x = pos.x + (node.pos * font->glyph_w),
                    .y = pos.y
                };
                font_render(font, renderer, pt, fg, it->text + node.pos, node.len);
            }

            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_code_end: { copy.line = 0; break; }

        case markup_link: {
            const struct font *font = doc->s.link.font;
            struct rgba fg = { 0 }, bg = { 0 };

            SDL_Rect rect = {
                .x = pos.x,
                .y = pos.y,
                .w = font->glyph_w * it->len,
                .h = font->glyph_h,
            };

            if (!ui_cursor_in(&rect)) {
                font = doc->s.link.font;
                fg = doc->s.link.fg;
                bg = doc->s.link.bg;
            }
            else if (!doc->pressed) {
                font = doc->s.hover.font;
                fg = doc->s.hover.fg;
                bg = doc->s.hover.bg;
            }
            else {
                font = doc->s.pressed.font;
                fg = doc->s.pressed.fg;
                bg = doc->s.pressed.bg;
            }

            font_render_bg(font, renderer, pos_as_point(pos), fg, bg, it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        default: { assert(false); }
        }
    }

    if (doc->copy.line) {
        constexpr size_t chars = 4;
        const struct font *font = doc->s.copy.font;

        SDL_Rect *rect = &doc->copy.rect;
        rect->w = (chars * font->glyph_w) + (doc->s.copy.margin * 2);
        rect->x = (inner.base.pos.x + inner.base.dim.w) - rect->w;
        rect->h = font->glyph_h;

        bool hover = ui_cursor_in(rect);
        bool pressed = ui_cursor_button_down(SDL_BUTTON_LEFT);

        struct rgba bg =
            (hover && pressed) ? doc->s.copy.pressed :
            hover ? doc->s.copy.hover :
            doc->s.copy.bg;

        rgba_render(bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, rect));

        SDL_Point pt = { .x = rect->x + doc->s.copy.margin, .y = rect->y };
        font_render(font, renderer, pt, doc->s.copy.fg, "copy", chars);
    }
}
