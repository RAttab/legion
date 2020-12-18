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

enum
{
    panel_code_count = 3,
    panel_code_sep = 2,
    panel_code_prefix = panel_code_count + panel_code_sep,
};

struct panel_code_state
{
    struct text text;
    struct mod *mod;
    size_t line_cap;
};

static struct font *panel_code_font(void) { return font_mono8; }

static void panel_code_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_code_state *state = state_;

    struct font *font = panel_code_font();
    font_reset(font);

    struct mod_err *err = state->mod->errs;
    struct mod_err *err_end = err + state->mod->errs_len;

    struct line *line = state->text.first;
    SDL_Point pos = { .x = rect->x, .y = rect->y };
    for (size_t i = 0; line; ++i, line = line->next) {

        if (err != err_end && err->line == i) {
            sdl_err(SDL_SetRenderDrawColor(renderer, 0xCC, 0x00, 0x00, 0x55));
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = pos.x, .y = pos.y,
                                .h = font->glyph_h, .w = rect->w }));
            err++;
        }

        sdl_err(SDL_SetTextureColorMod(font->tex, 0x00, 0x33, 0xCC));

        char count[panel_code_count] = {0};
        str_utoa(i, count, sizeof(count));
        font_render(font, renderer, count, sizeof(count), pos);
        pos.x += font->glyph_w * sizeof(count);

        char sep[] = ": ";
        font_render(font, renderer, sep, sizeof(sep), pos);
        pos.x += font->glyph_w * sizeof(sep);

        const uint8_t gray = 0xDD;
        size_t len = line_len(line);
        sdl_err(SDL_SetTextureColorMod(font->tex, gray, gray, gray));
        font_render(font, renderer, line->c, len, pos);

        pos.x = rect->x;
        pos.y += font->glyph_h;
    }
}

static bool panel_code_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_code_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {
        case EV_CODE_SELECT: {
            mod_t id = (uintptr_t) event->user.data1;
            state->mod = mods_load(id);
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
    text_clear(&state->text);
    mod_discard(state->mod);
    free(state);
};

struct panel *panel_code_new(void)
{
    size_t menu_h = panel_menu_height();

    struct font *font = panel_code_font();
    size_t inner_w = font->glyph_w * (panel_code_prefix + text_line_cap);
    size_t inner_h = core.rect.h - menu_h - panel_total_padding;

    int outer_w = 0, outer_h = 0;
    panel_add_borders(inner_w, inner_h, &outer_w, &outer_h);

    struct panel_code_state *state = calloc(1, sizeof(*state));
    state->line_cap = inner_w / font->glyph_h;

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = core.ui.mods->rect.w,
                .y = menu_h,
                .w = outer_w, .h = outer_h });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_code_render;
    panel->events = panel_code_events;
    panel->free = panel_code_free;

    return panel;
}
