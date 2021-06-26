/* input.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// input
// -----------------------------------------------------------------------------

static const struct dim ui_input_margin = { .w = 2, .h = 2 };

struct ui_input ui_input_new(struct font *font, size_t cap)
{
    struct ui_input input = {
        .w = ui_widget_new(
                font->glyph_w * cap + ui_input_margin.w * 2,
                font->glyph_h + ui_input_margin.h * 2),
        .font = font,
        .focused = false,
    };

    input.buf.c = calloc(cap + 1, 1);
    input.buf.len = 0;
    input.buf.cap = cap;
    input.carret.col = 0;

    return input;
}

void ui_input_free(struct ui_input *input)
{
    free(input->buf.c);
}

void ui_input_clear(struct ui_input *input)
{
    input->buf.len = 0;
    input->carret.col = 0;
}

void ui_input_set(struct ui_input *input, const char *str)
{
    input->buf.len = strnlen(str, input->buf.cap - 1);
    memcpy(input->buf.c, str, input->buf.len);
}

void ui_input_tick(struct ui_input *input, uint64_t ticks)
{
    bool blink = (ticks / 20) % 2;
    if (input->carret.blink != blink)
        input->carret.blink = blink;
}

// \todo could use better error handling
uint64_t ui_input_get_u64(struct ui_input *input)
{
    if (input->buf.c[0] == '@') {
        if (input->buf.len > vm_atom_cap) return 0;
        atom_t atom = {0};
        memcpy(atom, input->buf.c+1, input->buf.len-1);
        return vm_atom(&atom);
    }

    uint64_t val = 0;
    if (input->buf.c[0] == '0' && input->buf.c[1] == 'x')
        (void) str_atox(input->buf.c+2, input->buf.len-2, &val);
    else (void) str_atou(input->buf.c, input->buf.len, &val);
    return val;
}

// \todo could use better error handling
uint64_t ui_input_get_hex(struct ui_input *input)
{
    uint64_t val = 0;
    (void) str_atox(input->buf.c, input->buf.len, &val);
    return val;
}

void ui_input_render(
        struct ui_input *input, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &input->w);
    SDL_Rect rect = ui_widget_rect(&input->w);

    rgba_render(rgba_gray(0x33), renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    SDL_Point pos = {
        .x = rect.x + ui_input_margin.w,
        .y = rect.y + ui_input_margin.h,
    };
    font_render(input->font, renderer, pos, rgba_white(), input->buf.c, input->buf.len);

    if (input->focused && input->carret.blink) {
        size_t x = ui_input_margin.w + input->carret.col * input->font->glyph_w;
        size_t y = ui_input_margin.h;

        rgba_render(rgba_white(), renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = input->w.pos.x + x,
                            .y = input->w.pos.y + y,
                            .w = input->font->glyph_w,
                            .h = input->font->glyph_h,
                        }));
    }
}


// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

static enum ui_ret ui_input_event_click(struct ui_input *input)
{
    SDL_Point cursor = core.cursor.point;
    SDL_Rect rect = ui_widget_rect(&input->w);

    input->focused = sdl_rect_contains(&rect, &cursor);
    if (!input->focused) return ui_nil;

    size_t col = (cursor.x - input->w.pos.x) / input->font->glyph_w;
    assert(col < input->buf.cap);

    input->carret.col = legion_min(col, input->buf.len);
    return ui_consume;
}

static enum ui_ret ui_input_event_move(struct ui_input *input, int hori)
{
    if (hori > 0) {
        input->carret.col = legion_min(input->carret.col+1, input->buf.len);
        return ui_consume;
    }

    if (hori < 0) {
        if (input->carret.col > 0) input->carret.col--;
        return ui_consume;
    }

    return ui_nil;
}

static enum ui_ret ui_input_event_ins(struct ui_input *input, char key, uint16_t mod)
{
    assert(key != '\n');
    if (input->buf.len == input->buf.cap) return ui_consume;

    size_t col = input->carret.col;
    memmove(input->buf.c + col + 1, input->buf.c + col, input->buf.len - col);

    if (mod & KMOD_SHIFT) key = str_keycode_shift(key);
    input->buf.c[col] = key;

    input->buf.len++;
    input->carret.col++;
    return ui_consume;
}

static enum ui_ret ui_input_event_delete(struct ui_input *input)
{
    if (!input->buf.len) return ui_consume;

    size_t col = input->carret.col;
    if (col == input->buf.len) return ui_consume;

    memmove(input->buf.c + col, input->buf.c + col+1, input->buf.len - col-1);
    input->buf.len--;
    return ui_consume;
}

static enum ui_ret ui_input_event_backspace(struct ui_input *input)
{
    if (!input->buf.len) return ui_consume;

    size_t col = input->carret.col;
    if (!col) return ui_consume;

    memmove(input->buf.c + col-1, input->buf.c + col, input->buf.len - col);
    input->buf.len--;
    input->carret.col--;
    return ui_consume;
}

enum ui_ret ui_input_event(struct ui_input *input, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEBUTTONDOWN: { return ui_input_event_click(input); }

    case SDL_KEYDOWN: {
        if (!input->focused) return ui_nil;

        uint16_t mod = ev->key.keysym.mod;
        SDL_Keycode keysym = ev->key.keysym.sym;
        switch (keysym) {

        case SDLK_LEFT: { return ui_input_event_move(input, -1); }
        case SDLK_RIGHT: { return ui_input_event_move(input, 1); }

        // from 32 to 176 on the ascii table. The uppercase letters are not
        // mapped by SDL so they're just skipped
        case ' '...'~': { return ui_input_event_ins(input, keysym, mod); }

        case SDLK_DELETE: { return ui_input_event_delete(input); }
        case SDLK_BACKSPACE: { return ui_input_event_backspace(input); }

        case SDLK_UP:
        case SDLK_HOME: { input->carret.col = 0; return ui_consume; }

        case SDLK_DOWN:
        case SDLK_END: { input->carret.col = input->buf.len; return ui_consume; }

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }
}
