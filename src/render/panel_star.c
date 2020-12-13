/* panel_star.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "render/font.h"
#include "render/panel.h"
#include "render/core.h"
#include "game/coord.h"
#include "game/galaxy.h"
#include "utils/log.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// panel star
// -----------------------------------------------------------------------------

struct panel_star_state
{
    const struct star *star;

    size_t coord_y;
    size_t star_y;
    size_t elem_y;
};

static struct font *panel_star_font(void) { return font_mono6; }

enum {
    panel_star_val_len = 4,
    panel_star_elems_cols = 5,
    panel_star_elems_lines = elem_natural_len / panel_star_elems_cols + 1,
};

size_t panel_star_width(void)
{
    size_t glyphs =
        (2 + panel_star_val_len) * panel_star_elems_cols + // values
        (panel_star_elems_cols - 1); // spacing

    return glyphs * panel_star_font()->glyph_w;
}


static void star_val_str(char *dst, size_t len, size_t val)
{
    assert(len >= panel_star_val_len);

    static const char units[] = "ukMG?";

    size_t unit = 0;
    while (val > 1000) {
        val /= 1000;
        unit++;
    }

    dst[3] = units[unit];
    dst[2] = '0' + (val % 10);
    dst[1] = '0' + ((val / 10) % 10);
    dst[0] = '0' + ((val / 100) % 10);
}

static void panel_star_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_star_state *state = state_;
    struct font *font = panel_star_font();
    font_reset(font);

    {
        char str[coord_str_len+1] = {0};
        coord_str(state->star->coord, str, sizeof(str));
        font_render(font, renderer, str, coord_str_len, (SDL_Point) {
                    .x = rect->x, .y = rect->y + state->coord_y });
    }

    {
        static const char star_str[] = "power:"; // \0 will act as the space
        SDL_Point pos = { .x = rect->x, .y = rect->y + state->star_y };

        sdl_err(SDL_SetTextureColorMod(font->tex, 0xFF, 0xFF, 0xFF));
        font_render(font, renderer, star_str, sizeof(star_str), pos);
        pos.x += sizeof(star_str) * font->glyph_w;


        char val_str[panel_star_val_len];
        star_val_str(val_str, sizeof(val_str), state->star->power);

        uint8_t gray = 0x11 * (u64_log2(state->star->power) / 2);
        sdl_err(SDL_SetTextureColorMod(font->tex, gray, gray, gray));

        font_render(font, renderer, val_str, sizeof(val_str), pos);
    }

    {
        SDL_Point pos = { .x = rect->x, .y = rect->y + state->elem_y };

        for (size_t elem = 0; elem < elem_natural_len; ++elem) {
            if (elem == elem_natural_len - 1) pos.x += font->glyph_w * 7 * 2;

            char elem_str[] = {'A'+elem, ':'};
            sdl_err(SDL_SetTextureColorMod(font->tex, 0xFF, 0xFF, 0xFF));
            font_render(font, renderer, elem_str, sizeof(elem_str), pos);
            pos.x += sizeof(elem_str) * font->glyph_w;

            uint16_t value = state->star->elements[elem];

            char val_str[panel_star_val_len];
            star_val_str(val_str, sizeof(val_str), value);

            uint8_t gray = 0x11 * u64_log2(value);
            sdl_err(SDL_SetTextureColorMod(font->tex, gray, gray, gray));

            if (value) font_render(font, renderer, val_str, sizeof(val_str), pos);
            pos.x += sizeof(val_str) * font->glyph_w;

            if (elem % panel_star_elems_cols == panel_star_elems_cols - 1) {
                pos.x = rect->x;
                pos.y += font->glyph_h;
            }
            else pos.x += font->glyph_w;
        }
    }

    font_reset(font);
}

static bool panel_star_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_star_state *state = state_;
    if (event->type != core.event) return false;

    switch (event->user.code) {

    case EV_STAR_SELECT: {
        const struct star *star = event->user.data1;
        if (state->star && coord_eq(star->coord, state->star->coord))
            return false;

        state->star = star;
        panel_show(panel);
        break;
    }

    case EV_STAR_CLEAR: {
        state->star = NULL;
        panel_hide(panel);
        break;
    }

    }

    return false;
}

static void panel_star_free(void *state)
{
    free(state);
};

struct panel *panel_star_new(void)
{
    enum { spacing = 5 };

    struct font *font = panel_star_font();

    size_t inner_w = panel_star_width();
    size_t inner_h =
        (font->glyph_h + spacing) + // coord
        (font->glyph_h + spacing) + // power
        (panel_star_elems_lines * font->glyph_h + spacing); // elems

    int outer_w = 0, outer_h = 0;
    panel_add_borders(inner_w, inner_h, &outer_w, &outer_h);

    struct panel_star_state *state = calloc(1, sizeof(*state));
    state->coord_y = 0;
    state->star_y = state->coord_y + font->glyph_h + spacing;
    state->elem_y = state->star_y + font->glyph_h + spacing;

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = core.rect.w - outer_w,
                .y = panel_menu_height(),
                .w = outer_w, .h = outer_h });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_star_render;
    panel->events = panel_star_events;
    panel->free = panel_star_free;

    return panel;
}
