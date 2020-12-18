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
// panel_editor
// -----------------------------------------------------------------------------

struct panel_code_state
{
    struct layout *layout;
    atom_t name;
    struct text text;
    struct mod *mod;
    size_t line_cap;
};

enum
{
    p_code_mod = 0,
    p_code_mod_sep,
    p_code_text,
    p_code_len,
};

enum
{
    p_code_count = 3,
    p_code_sep = 2,
    p_code_prefix = p_code_count + p_code_sep,
};

static const char p_code_mod_str[] = "mod:";

static void panel_code_render_mod(
        struct panel_code_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_code_mod);

    font_render(layout->font, renderer, p_code_mod_str, sizeof(p_code_mod_str),
            layout_entry_pos(layout));
    font_render(layout->font, renderer, state->name, vm_atom_cap,
            layout_entry_index_pos(layout, 0, sizeof(p_code_mod_str)));
}

static void panel_code_render_text(
        struct panel_code_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_code_text);

    struct mod_err *err = state->mod->errs;
    struct mod_err *err_end = err + state->mod->errs_len;

    struct line *line = state->text.first;

    size_t rows = u64_min(state->text.len, layout->rows);
    for (size_t i = 0; line && i < rows; ++i, line = line->next) {
        SDL_Point pos = layout_entry_index_pos(layout, i, 0);

        if (err != err_end && err->line == i) {
            sdl_err(SDL_SetRenderDrawColor(renderer, 0xCC, 0x00, 0x00, 0x55));
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = pos.x, .y = pos.y,
                                .h = layout->item.h, .w = layout->rect.w }));
            err++;
        }

        sdl_err(SDL_SetTextureColorMod(layout->font->tex, 0x00, 0x33, 0xCC));

        char count[p_code_count] = {0};
        str_utoa(i, count, sizeof(count));
        font_render(layout->font, renderer, count, sizeof(count), pos);
        pos.x += layout->item.w * sizeof(count);

        char sep[] = ": ";
        font_render(layout->font, renderer, sep, sizeof(sep), pos);
        pos.x += layout->item.w * sizeof(sep);

        const uint8_t gray = 0xDD;
        size_t len = line_len(line);
        sdl_err(SDL_SetTextureColorMod(layout->font->tex, gray, gray, gray));
        font_render(layout->font, renderer, line->c, len, pos);
    }
}

static void panel_code_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_code_state *state = state_;
    (void) rect;

    panel_code_render_mod(state, renderer);
    panel_code_render_text(state, renderer);
}

static bool panel_code_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_code_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {
        case EV_CODE_SELECT: {
            mod_t id = (uintptr_t) event->user.data1;
            state->mod = mods_load(id);
            mods_name(id, &state->name);
            assert(state->mod);

            text_unpack(&state->text, state->mod->src, state->mod->src_len);
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

        default: { return false; }
        }
    }

    if (panel->hidden) return false;
    return false;
}

static void panel_code_free(void *state_)
{
    struct panel_code_state *state = state_;
    layout_free(state->layout);
    text_clear(&state->text);
    mod_discard(state->mod);
    free(state);
};

struct panel *panel_code_new(void)
{
    struct font *font = font_mono8;
    size_t menu_h = panel_menu_height();

    struct layout *layout = layout_alloc(p_code_len,
            core.rect.w, core.rect.h - menu_h - panel_total_padding);

    layout_text(layout, p_code_mod, font, sizeof(p_code_mod_str) + vm_atom_cap, 1);
    layout_sep(layout, p_code_mod_sep);
    layout_text(layout, p_code_text, font, p_code_prefix + text_line_cap, layout_inf);

    layout_finish(layout, (SDL_Point) { .x = panel_padding, .y = panel_padding });
    layout->pos = (SDL_Point) {
        .x = core.ui.mods->rect.w + panel_padding,
        .y = menu_h + panel_padding
    };

    struct panel_code_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = layout->pos.x - panel_padding,
                .y = layout->pos.y - panel_padding,
                .w = layout->bbox.w + panel_total_padding,
                .h = core.rect.h - menu_h });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_code_render;
    panel->events = panel_code_events;
    panel->free = panel_code_free;

    return panel;
}
