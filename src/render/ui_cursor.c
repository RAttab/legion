/* ui_cursor.c
   Rémi Attab (remi.attab@gmail.com), 04 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "render/ui.h"


// -----------------------------------------------------------------------------
// cursor
// -----------------------------------------------------------------------------

struct
{
    bool focus;
    size_t size;
    SDL_Point point;
    SDL_Texture *tex;
    bool buttons[5];
} ui_cursor = { 0 };


static bool ui_cursor_focus(void)
{
    uint32_t flags = SDL_GetWindowFlags(render.window);
    return flags & SDL_WINDOW_INPUT_FOCUS;
}

void ui_cursor_init(void)
{
    ui_cursor.size = 20;
    ui_cursor.point = (SDL_Point){
        .x = render.rect.w / 2,
        .y = render.rect.h / 2
    };

    ui_cursor.tex = db_img_cursor(render.renderer);
    sdl_err(SDL_SetTextureBlendMode(ui_cursor.tex, SDL_BLENDMODE_ADD));
    sdl_err(SDL_SetTextureColorMod(ui_cursor.tex, 0xFF, 0xFF, 0xFF));

    ui_cursor.focus = ui_cursor_focus();
    sdl_err(SDL_SetRelativeMouseMode(ui_cursor.focus));

    memset(&ui_cursor.buttons, 0, sizeof(ui_cursor.buttons));
}

void ui_cursor_free(void)
{
    SDL_DestroyTexture(ui_cursor.tex);
}

size_t ui_cursor_size(void)
{
    return ui_cursor.size;
}

struct pos ui_cursor_pos(void)
{
    return make_pos(ui_cursor.point.x, ui_cursor.point.y);
}

SDL_Point ui_cursor_point(void)
{
    return ui_cursor.point;
}

bool ui_cursor_in(const SDL_Rect* rect)
{
    return SDL_PointInRect(&ui_cursor.point, rect);
}

static size_t ui_cursor_button_index(uint8_t button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT: { return 0; }
    case SDL_BUTTON_MIDDLE: { return 1; }
    case SDL_BUTTON_RIGHT: { return 2; }
    case SDL_BUTTON_X1: { return 3; }
    case SDL_BUTTON_X2: { return 4; }
    default: { assert(false); }
    }
}

bool ui_cursor_button_down(uint8_t button)
{
    return ui_cursor.buttons[ui_cursor_button_index(button)];
}

void ui_cursor_update(void)
{
    bool focus = ui_cursor_focus();
    if (focus == ui_cursor.focus) return;

    sdl_err(SDL_SetRelativeMouseMode(focus));
    ui_cursor.focus = focus;
}

void ui_cursor_event(SDL_Event *ev)
{
    switch (ev->type)
    {

    case SDL_MOUSEMOTION: {
        ui_cursor.point.x += ev->motion.xrel;
        ui_cursor.point.x = legion_bound(ui_cursor.point.x, 0, render.rect.w);

        ui_cursor.point.y += ev->motion.yrel;
        ui_cursor.point.y = legion_bound(ui_cursor.point.y, 0, render.rect.h);
        return;
    }

    case SDL_MOUSEBUTTONDOWN: {
        ui_cursor.buttons[ui_cursor_button_index(ev->button.button)] = true;
        return;
    }

    case SDL_MOUSEBUTTONUP: {
        ui_cursor.buttons[ui_cursor_button_index(ev->button.button)] = false;
        return;
    }

    default: { return; }
    }
}

void ui_cursor_render(SDL_Renderer *renderer)
{
    if (!ui_cursor.focus) return;

    sdl_err(SDL_RenderCopy(renderer, ui_cursor.tex,
                    &(SDL_Rect){
                        .x = 0, .y = 0,
                        .w = 50, .h = 50 },
                    &(SDL_Rect){
                        .x = ui_cursor.point.x, .y = ui_cursor.point.y,
                        .w = ui_cursor.size, .h = ui_cursor.size }));
}
