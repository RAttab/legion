/* input.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "input.h"
#include "vm/mod.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_input_style_default(struct ui_style *s)
{
    s->input.base = (struct ui_input_style) {
        .pad = make_dim(2, 2),
        .font = s->font.base,
        .fg = s->rgba.fg,
        .bg = s->rgba.bg,
        .border = s->rgba.box.border,
        .carret = { .fg = s->rgba.carret, .blink = s->carret.blink },
    };

    s->input.line = s->input.base;
    s->input.line.pad.h = 0;
}


// -----------------------------------------------------------------------------
// input
// -----------------------------------------------------------------------------

struct ui_input ui_input_new_s(const struct ui_input_style *s, size_t len)
{
    assert(len < ui_input_cap);

    struct ui_input input = {
        .w = ui_widget_new(
                s->font->glyph_w * len + s->pad.w * 2,
                s->font->glyph_h + s->pad.h * 2),
        .s = *s,
        .p = ui_panel_current(),
        .focused = false,
    };

    input.view.col = 0;
    input.view.len = len;

    input.buf.c = calloc(ui_input_cap, sizeof(input.buf.c));
    input.buf.len = 0;

    input.carret = 0;

    return input;
}

struct ui_input ui_input_new(size_t len)
{
    return ui_input_new_s(&ui_st.input.base, len);
}

void ui_input_free(struct ui_input *input)
{
    free(input->buf.c);
}

void ui_input_focus(struct ui_input *input)
{
    render_push_event(ev_focus_input, (uintptr_t) input, 0);
}

static void ui_input_view_update(struct ui_input *input)
{
    if (input->carret < input->view.col)
        input->view.col = input->carret;

    if (input->carret >= input->view.col + input->view.len)
        input->view.col = (input->carret - input->view.len) + 1;
}

void ui_input_clear(struct ui_input *input)
{
    input->buf.len = 0;
    input->carret = 0;
    ui_input_view_update(input);

}
void ui_input_set(struct ui_input *input, const char *str)
{
    input->buf.len = strnlen(str, ui_input_cap);
    memcpy(input->buf.c, str, input->buf.len);

    input->carret = 0;
    ui_input_view_update(input);
}

size_t ui_input_get_str(struct ui_input *input, const char **str)
{
    const char *it = input->buf.c;
    const char *end = it + input->buf.len;
    it += str_skip_spaces(it, end - it);

    *str = it;
    return end - it;
}

bool ui_input_get_u64(struct ui_input *input, uint64_t *ret)
{
    const char *it = input->buf.c;
    const char *end = it + input->buf.len;
    it += str_skip_spaces(it, end - it);

    size_t read = str_atou(it, end - it, ret);
    return read > 0;
}

bool ui_input_get_hex(struct ui_input *input, uint64_t *ret)
{
    const char *it = input->buf.c;
    const char *end = it + input->buf.len;
    it += str_skip_spaces(it, end - it);

    size_t read = str_atox(it, end - it, ret);
    return read > 0;
}

bool ui_input_get_symbol(struct ui_input *input, struct symbol *ret)
{
    if (!input->buf.len) return false;
    return symbol_parse(input->buf.c, input->buf.len, ret) >= 0;
}

bool ui_input_eval(struct ui_input *input, vm_word *ret)
{
    if (!input->buf.len) return false;

    struct lisp_ret eval =
        proxy_eval(input->buf.c, input->buf.len);
    *ret = eval.value;

    if (!eval.ok)
        ui_log(st_error, "Invalid const LISP statement: '%s'", input->buf.c);
    return eval.ok;
}

void ui_input_render(
        struct ui_input *input, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &input->w);

    SDL_Rect rect = ui_widget_rect(&input->w);

    rgba_render(input->s.bg, renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    rgba_render(input->s.border, renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    SDL_Point pos = {
        .x = rect.x + input->s.pad.w,
        .y = rect.y + input->s.pad.h,
    };

    assert(input->view.col <= input->buf.len);
    const char *it = input->buf.c + input->view.col;
    size_t len = legion_min(input->buf.len - input->view.col, input->view.len);
    font_render(input->s.font, renderer, pos, input->s.fg, it, len);

    do {
        if (!input->focused) break;
        if (((ts_now() / input->s.carret.blink) % 2) == 0) break;

        size_t col = input->carret - input->view.col;
        size_t x = input->s.pad.w + col * input->s.font->glyph_w;
        size_t y = input->s.pad.h;

        rgba_render(input->s.carret.fg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = input->w.pos.x + x,
                            .y = input->w.pos.y + y,
                            .w = input->s.font->glyph_w,
                            .h = input->s.font->glyph_h,
                        }));
    } while (false);
}

// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

static enum ui_ret ui_input_event_click(struct ui_input *input)
{
    SDL_Point cursor = ui_cursor_point();
    SDL_Rect rect = ui_widget_rect(&input->w);

    input->focused = SDL_PointInRect(&cursor, &rect);
    if (!input->focused) return ui_nil;

    size_t col = (cursor.x - input->w.pos.x) / input->s.font->glyph_w;
    input->carret = legion_min(col, input->buf.len);

    render_push_event(ev_focus_input, (uintptr_t) input, 0);
    return ui_consume;
}

static enum ui_ret ui_input_event_move(struct ui_input *input, int hori)
{
    if (hori > 0)
        input->carret = legion_min(input->carret+1, input->buf.len);

    if (hori < 0 && input->carret > 0)
        input->carret--;

    ui_input_view_update(input);
    return ui_consume;
}

static enum ui_ret ui_input_event_ins(struct ui_input *input, char key, uint16_t mod)
{
    assert(key != '\n');
    if (input->buf.len == ui_input_cap-1) return ui_consume;

    size_t col = input->carret;
    memmove(input->buf.c + col + 1, input->buf.c + col, input->buf.len - col);

    if (mod & KMOD_SHIFT) key = str_keycode_shift(key);
    input->buf.c[col] = key;

    input->buf.len++;
    input->carret++;
    ui_input_view_update(input);
    return ui_consume;
}

static enum ui_ret ui_input_event_copy(struct ui_input *input)
{
    ui_clipboard_copy(input->buf.c, input->buf.len);
    return ui_consume;
}

static enum ui_ret ui_input_event_paste(struct ui_input *input)
{
    input->buf.len = ui_clipboard_paste(input->buf.c, ui_input_cap);
    input->carret = input->buf.len;
    ui_input_view_update(input);
    return ui_consume;
}

static enum ui_ret ui_input_event_delete(struct ui_input *input)
{
    if (!input->buf.len) return ui_consume;

    size_t col = input->carret;
    if (col == input->buf.len) return ui_consume;

    memmove(input->buf.c + col, input->buf.c + col+1, input->buf.len - col-1);
    input->buf.len--;
    return ui_consume;
}

static enum ui_ret ui_input_event_backspace(struct ui_input *input)
{
    if (!input->buf.len) return ui_consume;

    size_t col = input->carret;
    if (!col) return ui_consume;

    memmove(input->buf.c + col-1, input->buf.c + col, input->buf.len - col);
    input->buf.len--;
    input->carret--;
    ui_input_view_update(input);
    return ui_consume;
}


enum ui_ret ui_input_event(struct ui_input *input, const SDL_Event *ev)
{
    if (render_user_event_is(ev, ev_focus_input))
        input->focused = (input == ev->user.data1);

    switch (ev->type) {

    case SDL_MOUSEBUTTONDOWN: { return ui_input_event_click(input); }

    case SDL_KEYDOWN: {
        if (!input->focused) return ui_nil;

        uint16_t mod = ev->key.keysym.mod;
        SDL_Keycode keysym = ev->key.keysym.sym;
        switch (keysym) {

        case SDLK_RETURN: { return ui_action; }

        case SDLK_LEFT: { return ui_input_event_move(input, -1); }
        case SDLK_RIGHT: { return ui_input_event_move(input, 1); }

        // from 32 to 176 on the ascii table. The uppercase letters are not
        // mapped by SDL so they're just skipped
        case ' '...'~': {
            if (likely(!(mod & (KMOD_CTRL | KMOD_ALT))))
                return ui_input_event_ins(input, keysym, mod);

            if (mod & KMOD_CTRL) {
                switch (keysym)
                {
                case 'c': { return ui_input_event_copy(input); }
                case 'v': { return ui_input_event_paste(input); }
                default: { return ui_nil; }
                }
            }
        }

        case SDLK_DELETE: { return ui_input_event_delete(input); }
        case SDLK_BACKSPACE: { return ui_input_event_backspace(input); }

        case SDLK_UP:
        case SDLK_HOME: {
            input->carret = 0;
            ui_input_view_update(input);
            return ui_consume;
        }

        case SDLK_DOWN:
        case SDLK_END: {
            input->carret = input->buf.len;
            ui_input_view_update(input);
            return ui_consume;
        }

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }
}
