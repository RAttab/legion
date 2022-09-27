/* doc.c
   Rémi Attab (remi.attab@gmail.com), 11 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "game/man.h"


struct ui_doc ui_doc_new(struct dim dim, int pt)
{
    struct font *font = make_font(font_small, font_nil);

    struct ui_doc ui = {
        .w = ui_widget_new(dim.w, dim.h),
        .scroll = ui_scroll_new(dim, font->glyph_h),
        .cols = (dim.w / font->glyph_w) - 2,
        .font = {
            .pt = pt,
            .h = font->glyph_h,
            .w = font->glyph_w,
        },
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
        if (!sdl_rect_contains(&rect, &cursor)) return ui_nil;

        uint8_t col = (cursor.x - doc->w.pos.x) / doc->font.w;
        man_line line = (cursor.y - doc->w.pos.y) / doc->font.h;
        line += doc->scroll.first;

        struct link link = man_click(doc->man, line, col);
        if (link_is_nil(link)) return ui_nil;

        if (link.page == link_ui_tape)
            render_push_event(EV_TAPE_SELECT, link.section, 0);
        else render_push_event(EV_MAN_GOTO, link_to_u64(link), 0);

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
            pos.y += doc->font.h;
            line++;
            break;
        }

        case markup_text: {
            struct font *font = make_font(doc->font.pt, font_nil);
            font_render(font, renderer, pos_as_point(pos), rgba_white(), it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_underline: {
            struct font *font = make_font(doc->font.pt, font_nil);
            font_render(font, renderer, pos_as_point(pos), rgba_white(), it->text, it->len);

            size_t skip = 0;
            while (skip < it->len && it->text[skip] == ' ') skip++;

            int line_y = pos.y + font->glyph_baseline + 2;
            int line_start = pos.x + (skip * font->glyph_w);
            int line_end = pos.x + (it->len * font->glyph_w);

            rgba_render(rgba_white(), renderer);
            sdl_err(SDL_RenderDrawLine(renderer, line_start, line_y, line_end, line_y));

            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_bold: {
            struct font *font = make_font(doc->font.pt, font_bold);
            font_render(font, renderer, pos_as_point(pos), rgba_white(), it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_code: {
            struct font *font = make_font(doc->font.pt, font_nil);

            rgba_render(rgba_gray_a(0xFF, 0x33), renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = pos.x,
                                .y = pos.y,
                                .w = inner.base.dim.w - (pos.x - inner.base.pos.x),
                                .h = font->glyph_h,
                            }));

            font_render(font, renderer, pos_as_point(pos), rgba_white(), it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        case markup_link: {
            struct font *font = make_font(doc->font.pt, font_nil);

            SDL_Rect rect = {
                .x = pos.x,
                .y = pos.y,
                .w = font->glyph_w * it->len,
                .h = font->glyph_h,
            };
            struct rgba rgba = sdl_rect_contains(&rect, &render.cursor.point) ?
                make_rgba(0x00, 0x00, 0xCC, 0xFF) :
                make_rgba(0x00, 0x00, 0xFF, 0xFF);

            font_render(font, renderer, pos_as_point(pos), rgba, it->text, it->len);
            pos.x += it->len * font->glyph_w;
            break;
        }

        default: { assert(false); }
        }

        it = man_next_markup(doc->man, it);
    }
}
