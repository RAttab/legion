/* code.c
   Rémi Attab (remi.attab@gmail.com), 17 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "vm/mod.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct ui_code ui_code_new(struct dim dim, struct font *font)
{
    struct ui_code code = {
        .w = ui_widget_new(dim.w, dim.h),
        .scroll = ui_scroll_new(dim, font->glyph_h),
        .font = font,
        .focused = false,
        .cols = 1,
    };

    text_init(&code.text);

    return code;
}


void ui_code_free(struct ui_code *code)
{
    ui_scroll_free(&code->scroll);
    text_clear(&code->text);
}

void ui_code_clear(struct ui_code *code)
{
    code->mod = NULL;
    text_clear(&code->text);
}

void ui_code_set(struct ui_code *code, const struct mod *mod, ip_t ip)
{
    code->mod = mod;
    text_from_str(&code->text, mod->src, mod->src_len);

    code->carret.line = code->text.first;
    code->carret.row = code->carret.col = 0;
    ui_scroll_update(&code->scroll, code->text.lines);

    if (ip) {
        struct mod_index index = mod_index(code->mod, ip);
        for (size_t i = 0; i < index.row && code->carret.line->next; ++i)
            code->carret.line = code->carret.line->next;
        code->scroll.first = index.row;
        code->carret.col = index.col;
    }
}

void ui_code_render(
        struct ui_code *code, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_layout inner = ui_scroll_render(&code->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    struct font *font = code->font;
    code->w = code->scroll.w;
    code->cols = (inner.dim.w / font->glyph_w) - 5;

    size_t first = ui_scroll_first(&code->scroll);
    size_t last = first + code->scroll.visible;

    struct line *line = text_goto(&code->text, first);
    size_t row = first;
    size_t col = 0;

    struct mod_err *err = code->mod->errs;
    struct mod_err *err_end = err + code->mod->errs_len;

    for (size_t i = first; i < last && line; ++i) {

        // line number
        {
            struct ui_widget w = ui_widget_new(font->glyph_w * 4, font->glyph_h);
            ui_layout_add(&inner, &w);

            rgba_render(rgba_gray_a(0x44, 0x88), renderer);

            SDL_Rect rect = ui_widget_rect(&w);
            sdl_err(SDL_RenderFillRect(renderer, &rect));

            if (!col) {
                char str[4] = {0};
                str_utoa(row, str, sizeof(str));

                struct rgba fg = rgba_gray(0x88);
                SDL_Point pos = pos_as_point(w.pos);
                font_render(font, renderer, pos, fg, str, sizeof(str));
            }
        }

        ui_layout_sep_x(&inner, code->font->glyph_w);

        // code line
        struct ui_widget w = ui_widget_new(font->glyph_w * code->cols, font->glyph_h);
        ui_layout_add(&inner, &w);

        size_t len = legion_min(line->len - col, code->cols);

        // err
        while (err < err_end) {
            if (err->row < row) { err++; continue; }
            else if (row != err->row) break;

            if (err->col + err->len < col) { err++; continue; }
            if (err->col > col + len) break;

            size_t start = legion_max(err->col, col);
            size_t end = legion_min(err->col + len, col + len);

            rgba_render(make_rgba(0x88, 0x00, 0x00, 0x44), renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = w.pos.x + (start * font->glyph_w),
                                .y = w.pos.y,
                                .w = (end - start) * font->glyph_w,
                                .h = font->glyph_h,
                            }));
            err++;
        }

        // code text
        {
            struct rgba fg = rgba_white();
            SDL_Point pos = pos_as_point(w.pos);
            font_render(font, renderer, pos, fg, line->c + col, len);
        }

        // carret
        if (code->focused && code->carret.blink && code->carret.row == row &&
                (code->carret.col >= col && code->carret.col < col + len))
        {
            rgba_render(rgba_white(), renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = w.pos.x + ((code->carret.col - col) * font->glyph_w),
                                .y = w.pos.y,
                                .w = font->glyph_w,
                                .h = font->glyph_h,
                            }));
        }

        // advance
        if (col + len < line->len) col += len;
        else { row++; col = 0; line = line->next; }
        ui_layout_next_row(&inner);
    }
}


// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

static enum ui_ret ui_code_event_click(struct ui_code *code)
{
    SDL_Point cursor = core.cursor.point;
    SDL_Rect rect = ui_widget_rect(&code->w);

    code->focused = sdl_rect_contains(&rect, &cursor);
    if (!code->focused) return ui_nil;

    size_t col = (cursor.x - code->w.pos.x) / code->font->glyph_w;
    size_t row = (cursor.y - code->w.pos.y) / code->font->glyph_h;
    col = col < (ui_code_num_len+1) ? 0 : col - (ui_code_num_len+1);
    assert(row < code->scroll.visible);
    assert(col < code->cols);

    while (code->carret.row > row) {
        code->carret.line = code->carret.line->prev;
        code->carret.row--;
    }

    while (code->carret.row < row) {
        if (!code->carret.line->next) {
            code->carret.col = code->carret.line->len;
            return ui_consume;
        }
        code->carret.line = code->carret.line->next;
        code->carret.row++;
    }

    code->carret.col = legion_min(col, code->carret.line->len);

    core_push_event(EV_FOCUS_INPUT, (uintptr_t) code, 0);
    return ui_consume;
}

static enum ui_ret ui_code_event_move(struct ui_code *code, int hori, int vert)
{
    if (hori > 0) {
        if (code->carret.col < code->carret.line->len) code->carret.col++;
        else {
            if (!code->carret.line->next) return ui_nil;
            code->carret.line = code->carret.line->next;
            code->carret.col = 0;
            if (code->carret.row == code->scroll.visible-1) code->scroll.first++;
            else code->carret.row++;
        }
        return ui_consume;
    }

    if (hori < 0) {
        if (code->carret.col > 0) code->carret.col--;
        else {
            if (!code->carret.line->prev) return ui_nil;
            code->carret.line = code->carret.line->prev;
            code->carret.col = code->carret.line->len;
            if (!code->carret.row) code->scroll.first--;
            else code->carret.row--;
        }
        return ui_consume;
    }

    if (vert) {
        if (vert > 0) {
            if (!code->carret.line->next) return ui_nil;
            if (code->carret.row == code->scroll.visible-1) code->scroll.first++;
            else code->carret.row++;
            code->carret.line = code->carret.line->next;
        }

        if (vert < 0) {
            if (!code->carret.line->prev) return ui_nil;
            if (!code->carret.row) code->scroll.first--;
            else code->carret.row--;
            code->carret.line = code->carret.line->prev;
        }
        code->carret.col = legion_min(code->carret.col, code->carret.line->len);
        return ui_consume;
    }

    return ui_nil;
}

static void ui_code_event_scroll(struct ui_code *code, size_t old, size_t new)
{
    if (old == new) return;

    if (old < new) {
        size_t delta = new - old;
        for (size_t i = 0; i < delta; ++i) {
            if (code->carret.row) code->carret.row--;
            else code->carret.line = code->carret.line->next;
            assert(code->carret.line);
        }
    }
    else {
        size_t delta = old - new;
        for (size_t i = 0; i < delta; ++i) {
            if (code->carret.row < code->scroll.visible-1) code->carret.row++;
            else code->carret.line = code->carret.line->prev;
            assert(code->carret.line);
        }
    }
    code->carret.col = legion_min(code->carret.col, code->carret.line->len);

}

static enum ui_ret ui_code_event_ins(struct ui_code *code, char key, uint16_t mod)
{
    if (mod & KMOD_SHIFT) key = str_keycode_shift(key);

    struct line_ret ret =
        line_insert(&code->text, code->carret.line, code->carret.col, key);
    if (!ret.line) return ui_consume;

    code->carret.col = ret.index;
    if (ret.line == code->carret.line) return ui_consume;

    code->carret.line = ret.line;
    code->scroll.total = code->text.lines;

    if (code->carret.row == code->scroll.visible - 1) code->scroll.first++;
    else code->carret.row++;

    return ui_consume;
}

static enum ui_ret ui_code_event_delete(struct ui_code *code)
{
    struct line_ret ret =
        line_delete(&code->text, code->carret.line, code->carret.col);
    if (!ret.line) return ui_consume;

    code->carret.col = ret.index;
    if (ret.line == code->carret.line) return ui_consume;

    code->carret.line = ret.line;
    code->scroll.total = code->text.lines;
    return ui_consume;
}

static enum ui_ret ui_code_event_backspace(struct ui_code *code)
{
    struct line_ret ret =
        line_backspace(&code->text, code->carret.line, code->carret.col);
    if (!ret.line) return ui_consume;

    code->carret.col = ret.index;
    if (ret.line == code->carret.line) return ui_consume;

    code->carret.line = ret.line;
    code->scroll.total = code->text.lines;

    if (!code->carret.row) code->scroll.first--;
    else code->carret.row--;

    return ui_consume;
}

static enum ui_ret ui_code_event_user(struct ui_code *code, const SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_TICK: {
        uint64_t ticks = (uintptr_t) ev->user.data1;
        code->carret.blink = (ticks / 20) % 2;
        return ui_nil;
    }

    case EV_FOCUS_INPUT: {
        void *target = ev->user.data1;
        code->focused = target == code;
        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

enum ui_ret ui_code_event(struct ui_code *code, const SDL_Event *ev)
{
    if (ev->type == core.event) return ui_code_event_user(code, ev);

    enum ui_ret ret = ui_nil;

    size_t old_scroll = code->scroll.first;
    if ((ret = ui_scroll_event(&code->scroll, ev))) {
        if (ret != ui_skip) ui_code_event_scroll(code, old_scroll, code->scroll.first);
        return ret;
    }

    switch (ev->type) {

    case SDL_MOUSEBUTTONDOWN: { return ui_code_event_click(code); }

    case SDL_KEYDOWN: {
        if (!code->focused) return ui_nil;

        uint16_t mod = ev->key.keysym.mod;
        SDL_Keycode keysym = ev->key.keysym.sym;
        switch (keysym) {

        case SDLK_UP: { return ui_code_event_move(code, 0, -1); }
        case SDLK_DOWN: { return ui_code_event_move(code, 0, 1); }
        case SDLK_LEFT: { return ui_code_event_move(code, -1, 0); }
        case SDLK_RIGHT: { return ui_code_event_move(code, 1, 0); }

        // from 32 to 176 on the ascii table. The uppercase letters are not
        // mapped by SDL so they're just skipped
        case ' '...'~': { return ui_code_event_ins(code, keysym, mod); }
        case SDLK_RETURN: { return ui_code_event_ins(code, '\n', mod); }

        case SDLK_DELETE: { return ui_code_event_delete(code); }
        case SDLK_BACKSPACE: { return ui_code_event_backspace(code); }

        case SDLK_HOME: { code->carret.col = 0; return ui_consume; }
        case SDLK_END: { code->carret.col = code->carret.line->len; return ui_consume; }

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }
}
