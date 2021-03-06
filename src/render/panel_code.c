/* panel_editor.c
   Rémi Attab (remi.attab@gmail.com), 09 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"
#include "render/font.h"
#include "vm/mod.h"
#include "utils/text.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct pcode_state
{
    struct layout *layout;
    struct ui_scroll scroll;

    atom_t name;
    struct text text;
    struct mod *mod;

    struct {
        bool blink;
        size_t row, col;
        struct line *line;
    } carret;
};

enum
{
    pcode_mod = 0,
    pcode_mod_sep,
    pcode_text,
    pcode_len,
};

enum
{
    pcode_count = 3,
    pcode_sep = 2,

    pcode_prefix_len = pcode_count + pcode_sep,
    // + 1 -> need a trailing space for the carret.
    pcode_text_len = pcode_prefix_len + text_line_cap + 1,
    pcode_text_total_len = pcode_text_len + ui_scroll_layout_cols,
};

static const char pcode_mod_str[] = "mod:";


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void pcode_render_mod(
        struct pcode_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, pcode_mod);

    font_render(layout->font, renderer, pcode_mod_str, sizeof(pcode_mod_str),
            layout_entry_pos(layout));
    font_render(layout->font, renderer, state->name, vm_atom_cap,
            layout_entry_index_pos(layout, 0, sizeof(pcode_mod_str)));
}

static void pcode_render_text(
        struct pcode_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, pcode_text);

    struct mod_err *err = state->mod->errs;
    struct mod_err *err_end = err + state->mod->errs_len;

    struct line *line = state->text.first;

    const size_t first = state->scroll.first;
    const size_t rows = u64_min(state->text.len, state->scroll.visible);
    for (size_t i = 0; line && i < first; ++i) line = line->next;

    for (size_t i = first; line && i < first + rows; ++i, line = line->next) {
        SDL_Point pos = layout_entry_index_pos(layout, i - first, 0);

        if (err != err_end && err->line == i) {
            sdl_err(SDL_SetRenderDrawColor(renderer, 0xCC, 0x00, 0x00, 0x55));
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = pos.x, .y = pos.y,
                                .h = layout->item.h, .w = layout->rect.w }));
            err++;
        }

        sdl_err(SDL_SetTextureColorMod(layout->font->tex, 0x00, 0x33, 0xCC));

        char count[pcode_count] = {0};
        str_utoa(i, count, sizeof(count));
        font_render(layout->font, renderer, count, sizeof(count), pos);
        pos.x += layout->item.w * sizeof(count);

        char sep[] = ":";
        font_render(layout->font, renderer, sep, sizeof(sep), pos);
        pos.x += layout->item.w * sizeof(sep);

        const uint8_t gray = 0xDD;
        size_t len = line_len(line);
        sdl_err(SDL_SetTextureColorMod(layout->font->tex, gray, gray, gray));
        font_render(layout->font, renderer, line->c, len, pos);
    }

    if (state->carret.blink) {
        sdl_err(SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF));
        SDL_Rect rect = layout_entry_index(
                layout, state->carret.row, state->carret.col + pcode_prefix_len);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    ui_scroll_render(&state->scroll, renderer,
            layout_entry_index_pos(layout, 0, pcode_text_len));
}

static void pcode_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct pcode_state *state = state_;
    (void) rect;

    pcode_render_mod(state, renderer);
    pcode_render_text(state, renderer);
}

// -----------------------------------------------------------------------------
// carret
// -----------------------------------------------------------------------------

static bool pcode_carret_click(struct pcode_state *state)
{
    SDL_Rect abs = layout_abs(state->layout, pcode_text);
    if (!sdl_rect_contains(&abs, &core.cursor.point)) return false;

    struct layout_entry *layout = layout_entry(state->layout, pcode_text);

    SDL_Point rel = {
        .x = core.cursor.point.x - abs.x,
        .y = core.cursor.point.y - abs.y,
    };
    assert(rel.x < abs.w && rel.y < abs.h);

    size_t row = 0, col = 0;
    layout_entry_point(layout, rel, &row, &col);
    col = col > pcode_prefix_len ? col - pcode_prefix_len : 0;

    while (state->carret.row > row) {
        state->carret.line = state->carret.line->prev;
        state->carret.row--;
    }

    while (state->carret.row < row) {
        if (!state->carret.line->next) {
            state->carret.col = line_len(state->carret.line);
            return true;
        }
        state->carret.line = state->carret.line->next;
        state->carret.row++;
    }

    state->carret.col = u64_min(col, line_len(state->carret.line));
    return true;
}

static bool pcode_carret_move(struct pcode_state *state, int hori, int vert)
{
    if (hori > 0) {
        if (state->carret.col < line_len(state->carret.line)) state->carret.col++;
        else {
            if (!state->carret.line->next) return false;
            state->carret.line = state->carret.line->next;
            state->carret.col = 0;
            if (state->carret.row == state->scroll.visible-1) state->scroll.first++;
            else state->carret.row++;
        }
        return true;
    }

    if (hori < 0) {
        if (state->carret.col > 0) state->carret.col--;
        else {
            if (!state->carret.line->prev) return false;
            state->carret.line = state->carret.line->prev;
            state->carret.col = line_len(state->carret.line);
            if (!state->carret.row) state->scroll.first--;
            else state->carret.row--;
        }
        return true;
    }

    if (vert) {
        if (vert > 0) {
            if (!state->carret.line->next) return false;
            if (state->carret.row == state->scroll.visible-1) state->scroll.first++;
            else state->carret.row++;
            state->carret.line = state->carret.line->next;
        }

        if (vert < 0) {
            if (!state->carret.line->prev) return false;
            if (!state->carret.row) state->scroll.first--;
            else state->carret.row--;
            state->carret.line = state->carret.line->prev;
        }
        state->carret.col = u64_min(state->carret.col, line_len(state->carret.line));
        return true;
    }

    return false;
}

static void pcode_carret_scroll(
        struct pcode_state *state, size_t old, size_t new)
{
    assert(old != new);

    if (old < new) {
        size_t delta = new - old;
        for (size_t i = 0; i < delta; ++i) {
            if (state->carret.row) state->carret.row--;
            else state->carret.line = state->carret.line->next;
            assert(state->carret.line);
        }
    }
    else {
        size_t delta = old - new;
        for (size_t i = 0; i < delta; ++i) {
            if (state->carret.row < state->scroll.visible-1) state->carret.row++;
            else state->carret.line = state->carret.line->prev;
            assert(state->carret.line);
        }
    }
    state->carret.col = u64_min(state->carret.col, line_len(state->carret.line));
}

static bool pcode_carret_ins(
        struct pcode_state *state, char key, uint16_t mod)
{
    if (mod & KMOD_SHIFT) key = str_keycode_shift(key);

    struct line_ret ret =
        line_insert(&state->text, state->carret.line, state->carret.col, key);
    if (!ret.line) return true;

    state->carret.col = ret.index;
    if (ret.line == state->carret.line) return true;

    state->carret.line = ret.line;
    state->scroll.total = state->text.len;

    if (state->carret.row == state->scroll.visible - 1) state->scroll.first++;
    else state->carret.row++;

    return true;
}

static bool pcode_carret_delete(struct pcode_state *state)
{
    struct line_ret ret =
        line_delete(&state->text, state->carret.line, state->carret.col);
    if (!ret.line) return true;

    state->carret.col = ret.index;
    if (ret.line == state->carret.line) return true;

    state->carret.line = ret.line;
    state->scroll.total = state->text.len;
    return true;
}

static bool pcode_carret_backspace(struct pcode_state *state)
{
    struct line_ret ret =
        line_backspace(&state->text, state->carret.line, state->carret.col);
    if (!ret.line) return true;

    state->carret.col = ret.index;
    if (ret.line == state->carret.line) return true;

    state->carret.line = ret.line;
    state->scroll.total = state->text.len;

    if (!state->carret.row) state->scroll.first--;
    else state->carret.row--;

    return true;
}

static bool pcode_events_text(struct pcode_state *state, SDL_Event *event)
{
    switch (event->type) {

    case SDL_MOUSEBUTTONDOWN: { return pcode_carret_click(state); }

    case SDL_KEYDOWN: {
        uint16_t mod = event->key.keysym.mod;
        SDL_Keycode keysym = event->key.keysym.sym;
        switch (keysym) {

        case SDLK_UP: { return pcode_carret_move(state, 0, -1); }
        case SDLK_DOWN: { return pcode_carret_move(state, 0, 1); }
        case SDLK_LEFT: { return pcode_carret_move(state, -1, 0); }
        case SDLK_RIGHT: { return pcode_carret_move(state, 1, 0); }

        // from 32 to 176 on the ascii table. The uppercase letters are not
        // mapped by SDL so they're just skipped
        case ' '...'~': { return pcode_carret_ins(state, keysym, mod); }
        case SDLK_RETURN: { return pcode_carret_ins(state, '\n', mod); }

        case SDLK_DELETE: { return pcode_carret_delete(state); }
        case SDLK_BACKSPACE: { return pcode_carret_backspace(state); }

        case SDLK_HOME: { state->carret.col = 0; return true; }
        case SDLK_END: { state->carret.col = line_len(state->carret.line); return true; }

        default: { return false; }
        }
    }

    default: { return false; }
    }
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static bool pcode_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct pcode_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {
        case EV_CODE_SELECT: {
            mod_t id = (uintptr_t) event->user.data1;
            state->mod = mods_load(id);
            mods_name(id, &state->name);
            assert(state->mod);

            text_unpack(&state->text, state->mod->src, state->mod->src_len);
            state->carret.line = state->text.first;
            state->carret.row = state->carret.col = 0;
            ui_scroll_update(&state->scroll, state->text.len);

            ip_t ip = (uintptr_t) event->user.data2;
            size_t line = mod_line(state->mod, ip);
            for (size_t i = 0; i < line; ++i)
                state->carret.line = state->carret.line->next;
            state->scroll.first = line;

            panel_show(panel);
            return false;
        }

        case EV_CODE_CLEAR: {
            state->mod = NULL;
            mod_discard(state->mod);
            text_clear(&state->text);
            panel_hide(panel);
            return false;
        }

        case EV_STATE_UPDATE: {
            bool blink = (core.ticks / 20) % 2;
            if (state->carret.blink != blink) {
                state->carret.blink = blink;
                panel_invalidate(panel);
            }
            return false;
        }

        default: { return false; }
        }
    }

    if (panel->hidden) return false;

    {
        size_t old = state->scroll.first;
        enum ui_ret ret = ui_scroll_events(&state->scroll, event);
        if (ret & ui_action) pcode_carret_scroll(state, old, state->scroll.first);
        if (ret & ui_invalidate) panel_invalidate(panel);
        if (ret & ui_consume) return true;
    }

    if (pcode_events_text(state, event)) {
        panel_invalidate(panel);
        return true;
    }

    return false;
}


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

static void pcode_free(void *state_)
{
    struct pcode_state *state = state_;
    layout_free(state->layout);
    text_clear(&state->text);
    mod_discard(state->mod);
    free(state);
};

struct panel *panel_code_new(void)
{
    struct font *font = font_mono8;
    size_t menu_h = panel_menu_height();

    struct layout *layout = layout_alloc(pcode_len,
            core.rect.w, core.rect.h - menu_h - panel_total_padding);

    layout_text(layout, pcode_mod, font, sizeof(pcode_mod_str) + vm_atom_cap, 1);
    layout_sep(layout, pcode_mod_sep);

    layout_text(layout, pcode_text, font, pcode_text_total_len, layout_inf);

    layout_finish(layout, (SDL_Point) { .x = panel_padding, .y = panel_padding });
    layout->pos = (SDL_Point) { .x = core.ui.mods->rect.w, .y = menu_h };

    struct pcode_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    {
        SDL_Rect events = layout_abs(layout, pcode_text);
        SDL_Rect bar = layout_abs_index(layout, pcode_text, layout_inf, pcode_text_len);
        ui_scroll_init(&state->scroll, &bar, &events, 0,
                layout_entry(layout, pcode_text)->rows);
    }

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = layout->pos.x, .y = layout->pos.y,
                .w = layout->bbox.w + panel_total_padding,
                .h = layout->bbox.h + panel_total_padding });
    panel->hidden = true;
    panel->state = state;
    panel->render = pcode_render;
    panel->events = pcode_events;
    panel->free = pcode_free;
    return panel;
}
