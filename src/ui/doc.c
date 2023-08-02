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
        },

#define make_from(src) { .font = s->font.base, .fg = (src).fg, .bg = (src).bg }
        .link =    make_from(s->rgba.link.idle),
        .hover =   make_from(s->rgba.link.hover),
        .pressed = make_from(s->rgba.link.pressed),
#undef make_from

        .underline = { .fg = s->rgba.fg, .offset = 2 },
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
        .scroll = ui_scroll_new(dim, font->glyph_h),
        .cols = (dim.w / font->glyph_w) - 2,
    };

    return ui;
}

void ui_doc_free(struct ui_doc *doc)
{
    ui_scroll_free(&doc->scroll);
    if (doc->man) man_free(doc->man);
}


void ui_doc_open(struct ui_doc *doc, struct link link, struct lisp *lisp)
{
    struct man *man = man_open(link.page, doc->cols, lisp);
    if (!man) { render_log(st_error, "unknown man link"); return; }

    if (doc->man) man_free(doc->man);
    doc->man = man;

    ui_scroll_update(&doc->scroll, man_lines(doc->man));
    doc->scroll.first = man_section_line(doc->man, link.section);
}

enum ui_ret ui_doc_event(struct ui_doc *doc, const SDL_Event *ev)
{
    enum ui_ret ret = 0;
    if ((ret = ui_scroll_event(&doc->scroll, ev))) return ret;

    if (!doc->man) return ui_nil;

    switch (ev->type)
    {

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point cursor = render.cursor.point;
        SDL_Rect rect = ui_widget_rect(&doc->w);
        if (!SDL_PointInRect(&cursor, &rect)) return ui_nil;

        doc->pressed = true;
        return ui_consume;
    }

    case SDL_MOUSEBUTTONUP: {
        doc->pressed = false;

        SDL_Point cursor = render.cursor.point;
        SDL_Rect rect = ui_widget_rect(&doc->w);
        if (!SDL_PointInRect(&cursor, &rect)) return ui_nil;

        const struct font *font = doc->s.text.font;
        uint8_t col = (cursor.x - doc->w.pos.x) / font->glyph_w;
        man_line line = (cursor.y - doc->w.pos.y) / font->glyph_h;
        line += doc->scroll.first;

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
        case SDLK_UP: { ui_scroll_move(&doc->scroll, -1); return ui_consume; }
        case SDLK_DOWN: { ui_scroll_move(&doc->scroll, 1); return ui_consume;  }

        case SDLK_PAGEUP: {
            ui_scroll_move(&doc->scroll, -doc->scroll.visible);
            return true;
        }
        case SDLK_PAGEDOWN: {
            ui_scroll_move(&doc->scroll, doc->scroll.visible);
            return true;
        }

        case SDLK_HOME: { doc->scroll.first = 0; return ui_consume; }
        case SDLK_END: { doc->scroll.first = doc->scroll.total - 1; return ui_consume; }

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
    man_line line = doc->scroll.first;
    const man_line end = line + doc->scroll.visible;
    const struct markup *it = man_line_markup(doc->man, line);

    while (it && line < end) {
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

        case markup_code: {
            const struct font *font = doc->s.code.font;

            rgba_render(doc->s.code.bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = pos.x,
                                .y = pos.y,
                                .w = inner.base.dim.w - (pos.x - inner.base.pos.x),
                                .h = font->glyph_h,
                            }));

            font_render(
                    font,
                    renderer,
                    pos_as_point(pos),
                    doc->s.code.fg,
                    it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_link: {
            const struct font *font = doc->s.link.font;
            struct rgba fg = { 0 }, bg = { 0 };

            SDL_Rect rect = {
                .x = pos.x,
                .y = pos.y,
                .w = font->glyph_w * it->len,
                .h = font->glyph_h,
            };

            if (!SDL_PointInRect(&render.cursor.point, &rect)) {
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

        it = man_next_markup(doc->man, it);
    }
}
