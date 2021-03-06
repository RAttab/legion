/* panel.c
   Rémi Attab (remi.attab@gmail.com), 11 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"

#include "render/core.h"
#include "render/font.h"
#include "game/coord.h"
#include "utils/log.h"


// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

void panel_add_borders(int width, int height, int *dst_width, int *dst_height)
{
    *dst_width = width + panel_total_padding;
    *dst_height = height + panel_total_padding;
}

struct panel *panel_new(const SDL_Rect *rect)
{
    struct panel *panel = calloc(1, sizeof(*panel));
    panel->redraw = true;
    panel->hidden = true;
    panel->rect = *rect;
    panel->inner_rect = (SDL_Rect) {
        .x = panel_padding,
        .y = panel_padding,
        .w = rect->w - panel_total_padding,
        .h = rect->h - panel_total_padding,
    };

    panel->tex = sdl_ptr(SDL_CreateTexture(
            core.renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            panel->rect.w, panel->rect.h));
    sdl_err(SDL_SetTextureBlendMode(panel->tex, SDL_BLENDMODE_ADD));

    return panel;
}

void panel_free(struct panel *panel)
{
    if (panel->free) panel->free(panel->state);
    SDL_DestroyTexture(panel->tex);
    free(panel);
}

void panel_invalidate(struct panel *panel)
{
    panel->redraw = true;
}

void panel_hide(struct panel *panel)
{
    panel->hidden = true;
}

void panel_show(struct panel *panel)
{
    panel->hidden = false;
    panel->redraw = true;
}

SDL_Point panel_relative_point(struct panel *panel, const SDL_Point *point)
{
    return (SDL_Point) {
        .x = point->x - (panel->rect.x + panel->inner_rect.x),
        .y = point->y - (panel->rect.y + panel->inner_rect.y)};
}

bool panel_event(struct panel *panel, SDL_Event *event)
{
    if (panel->events)
        return panel->events(panel->state, panel, event);
    return false;
}

void panel_render(struct panel *panel, SDL_Renderer *renderer)
{
    if (panel->hidden) return;
    if (panel->redraw) {
        panel->redraw = false;

        sdl_err(SDL_SetRenderTarget(renderer, panel->tex));

        sdl_err(SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE));
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = 0, .y = 0,
                            .w = panel->rect.w,
                            .h = panel->rect.h }));

        const uint8_t gray = 0x10;
        sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, 0x88));
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = panel_border, .y = panel_border,
                            .w = panel->rect.w - panel_border * 2,
                            .h = panel->rect.h - panel_border * 2}));

        if (panel->render) {
            panel->render(panel->state, renderer, &panel->inner_rect);
        }

        sdl_err(SDL_SetRenderTarget(renderer, NULL));
    }

    sdl_err(SDL_RenderCopy(renderer, panel->tex, NULL, &panel->rect));
}
