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

enum { code_num_len = 4 + 1 };

struct code *code_new(struct dim dim, struct font *font)
{
    struct code *code = calloc(1, sizeof(*code));
    *code = (struct code) {
        .w = make_widget(dim.w, dim.h),
        .font = font,
        .scroll = scroll_new(dim, 1, 0),
        .num = label_var(font, code_num_len),
        .code = label_var(font, text_line_cap),
    };

    code->num->fg = rgba_gray(0x88);
    code->num->bg = rgba_gray_a(0x44, 0x88);

    text_init(&code->text);
    return code;
}


void code_free(struct code *code)
{
    scroll_free(code->scroll);
    text_clear(&code->text);
    free(code);
}

void code_set(struct code *code, struct mod *mod)
{
    code->mod = mod;
    text_from_str(&code->text, mod->src, mod->src_len);
    scroll_update(code->scroll, code->text.len);
}

void code_render(struct code *code, struct layout *layout, SDL_Renderer *renderer)
{
    struct layout inner = scroll_render(code->scroll, layout, renderer);
    if (layout_is_nil(&inner)) return;

    code->scroll->visible = inner.dim.h / code->font->glyph_h;
    size_t first = scroll_first(code->scroll);
    size_t last = scroll_last(code->scroll);

    struct line *line = text_goto(&code->text, first);
    struct mod_err *err = code->mod->errs;
    struct mod_err *err_end = err + code->mod->errs_len;

    for (size_t i = first; i < last; ++i, line = line->next) {
        {
            char num[code_num_len + 1] = {0};
            str_utoa(i, num, code_num_len - 1);
            num[code_num_len] = ':';

            label_set(code->num, num, sizeof(num));
            label_render(code->num, &inner, renderer);
        }

        {
            code->code->bg = rgba_nil();

            while (err->line < i && err < err_end) err++;
            if (err != err_end && err->line == i)
                code->code->bg = make_rgba(0x88, 0x00, 0x00, 0x44);

            label_set(code->code, line->c, line_len(line));
            label_render(code->code, &inner, renderer);
        }

        layout_next_row(&inner);
    }

    if (code->carret.blink) {
        size_t x = (code->carret.col + code_num_len) * code->font->glyph_w;
        size_t y = code->carret.row * code->font->glyph_h;

        rgba_render(rgba_white(), renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = inner.top.x + x, .y = inner.top.y + y,
                            .w = code->font->glyph_w, .h = code->font->glyph_h,
                        }));
    }
}


// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

static enum ui_ret code_event_click(struct code *code)
{
    SDL_Point cursor = core.cursor.point;
    SDL_Rect rect = widget_rect(&code->w);
    if (!sdl_rect_contains(&rect, &cursor)) return ui_nil;

    size_t col = (cursor.y - code->w.pos.y) / code->font->glyph_h;
    size_t row = (cursor.x - code->w.pos.x) / code->font->glyph_w;
    row = row < code_num_len ? 0 : row - code_num_len;
    assert(row < code->scroll->visible);
    assert(col < text_line_cap);

    while (code->carret.row > row) {
        code->carret.line = code->carret.line->prev;
        code->carret.row--;
    }

    while (code->carret.row < row) {
        if (!code->carret.line->next) {
            code->carret.col = line_len(code->carret.line);
            return ui_consume;
        }
        code->carret.line = code->carret.line->next;
        code->carret.row++;
    }

    code->carret.col = legion_min(col, line_len(code->carret.line));
    return ui_consume;
}

static enum ui_ret code_event_move(struct code *code, int hori, int vert)
{
    if (hori > 0) {
        if (code->carret.col < line_len(code->carret.line)) code->carret.col++;
        else {
            if (!code->carret.line->next) return ui_nil;
            code->carret.line = code->carret.line->next;
            code->carret.col = 0;
            if (code->carret.row == code->scroll->visible-1) code->scroll->first++;
            else code->carret.row++;
        }
        return ui_consume;
    }

    if (hori < 0) {
        if (code->carret.col > 0) code->carret.col--;
        else {
            if (!code->carret.line->prev) return ui_nil;
            code->carret.line = code->carret.line->prev;
            code->carret.col = line_len(code->carret.line);
            if (!code->carret.row) code->scroll->first--;
            else code->carret.row--;
        }
        return ui_consume;
    }

    if (vert) {
        if (vert > 0) {
            if (!code->carret.line->next) return ui_nil;
            if (code->carret.row == code->scroll->visible-1) code->scroll->first++;
            else code->carret.row++;
            code->carret.line = code->carret.line->next;
        }

        if (vert < 0) {
            if (!code->carret.line->prev) return ui_nil;
            if (!code->carret.row) code->scroll->first--;
            else code->carret.row--;
            code->carret.line = code->carret.line->prev;
        }
        code->carret.col = legion_min(code->carret.col, line_len(code->carret.line));
        return ui_consume;
    }

    return ui_nil;
}

static void code_event_scroll(struct code *code, size_t old, size_t new)
{
    assert(old != new);

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
            if (code->carret.row < code->scroll->visible-1) code->carret.row++;
            else code->carret.line = code->carret.line->prev;
            assert(code->carret.line);
        }
    }
    code->carret.col = legion_min(code->carret.col, line_len(code->carret.line));

}

static enum ui_ret code_event_ins(struct code *code, char key, uint16_t mod)
{
    if (mod & KMOD_SHIFT) key = str_keycode_shift(key);

    struct line_ret ret =
        line_insert(&code->text, code->carret.line, code->carret.col, key);
    if (!ret.line) return ui_consume;

    code->carret.col = ret.index;
    if (ret.line == code->carret.line) return ui_consume;

    code->carret.line = ret.line;
    code->scroll->total = code->text.len;

    if (code->carret.row == code->scroll->visible - 1) code->scroll->first++;
    else code->carret.row++;

    return ui_consume;
}

static enum ui_ret code_event_delete(struct code *code)
{
    struct line_ret ret =
        line_delete(&code->text, code->carret.line, code->carret.col);
    if (!ret.line) return ui_consume;

    code->carret.col = ret.index;
    if (ret.line == code->carret.line) return ui_consume;

    code->carret.line = ret.line;
    code->scroll->total = code->text.len;
    return ui_consume;
}

static enum ui_ret code_event_backspace(struct code *code)
{
    struct line_ret ret =
        line_backspace(&code->text, code->carret.line, code->carret.col);
    if (!ret.line) return ui_consume;

    code->carret.col = ret.index;
    if (ret.line == code->carret.line) return ui_consume;

    code->carret.line = ret.line;
    code->scroll->total = code->text.len;

    if (!code->carret.row) code->scroll->first--;
    else code->carret.row--;

    return ui_consume;
}

enum ui_ret code_event(struct code *code, const SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;

    size_t old_scroll = code->scroll->first;
    if ((ret = scroll_event(code->scroll, ev))) {
        if (ret != ui_skip) code_event_scroll(code, old_scroll, code->scroll->first);
        return ret;
    }

    switch (ev->type) {

    case SDL_MOUSEBUTTONDOWN: { return code_event_click(code); }

    case SDL_KEYDOWN: {
        uint16_t mod = ev->key.keysym.mod;
        SDL_Keycode keysym = ev->key.keysym.sym;
        switch (keysym) {

        case SDLK_UP: { return code_event_move(code, 0, -1); }
        case SDLK_DOWN: { return code_event_move(code, 0, 1); }
        case SDLK_LEFT: { return code_event_move(code, -1, 0); }
        case SDLK_RIGHT: { return code_event_move(code, 1, 0); }

        // from 32 to 176 on the ascii table. The uppercase letters are not
        // mapped by SDL so they're just skipped
        case ' '...'~': { return code_event_ins(code, keysym, mod); }
        case SDLK_RETURN: { return code_event_ins(code, '\n', mod); }

        case SDLK_DELETE: { return code_event_delete(code); }
        case SDLK_BACKSPACE: { return code_event_backspace(code); }

        case SDLK_HOME: { code->carret.col = 0; return ui_consume; }
        case SDLK_END: { code->carret.col = line_len(code->carret.line); return ui_consume; }

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }
}
