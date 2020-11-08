/* ui.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui.h"

#include "coord.h"
#include "render.h"
#include "sector.h"


struct ui_core
{
    struct sector *sector;
    SDL_Rect rect;

    int8_t scale;
    struct coord pos;

    SDL_Texture* tex;
};

struct ui_core *
ui_core_init(SDL_Renderer *renderer, struct sector *sector, SDL_Rect rect)
{
    struct ui_core *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_core) {
        .sector = sector,
        .rect = rect,
        .scale = 0,
        .pos = (struct coord) {
            .x = sector->coord.x + coord_sector_max / 2,
            .y = sector->coord.y + coord_sector_max / 2,
        },
        0
    };

    SDL_Surface *surface = SDL_LoadBMP("./res/core.bmp");
    if (surface == NULL) {
        SDL_Log("unable to load texture: %s", SDL_GetError());
        goto fail_bmp;
    }

    ui->tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (ui->tex == NULL) {
        SDL_Log("unable create texture: %s", SDL_GetError());
        goto fail_tex;
    }

    return ui;

    SDL_DestroyTexture(ui->tex);
  fail_tex:
    SDL_FreeSurface(surface);
  fail_bmp:
    free(ui);
    return NULL;
}

void ui_core_free(struct ui_core *ui)
{
    SDL_DestroyTexture(ui->tex);
    free(ui);
}

int64_t scale_mult(int8_t scale, int64_t value)
{
    if (scale > 0) {
        return value * (1UL << scale);
    }
    if (scale < 0) {
        return value / -(1UL << scale);
    }
    return value;
}

int64_t scale_div(int8_t scale, int64_t value)
{
    if (scale > 0) {
        return value / (1UL << scale);
    }
    if (scale < 0) {
        return value * -(1UL << scale);
    }
    return value;
}

struct coord core_project_coord(struct ui_core *ui, int x, int y)
{
    int64_t rel_x = scale_mult(ui->scale, x - ui->rect.w / 2);
    int64_t rel_y = scale_mult(ui->scale, y - ui->rect.h / 2);
    return coord(
            rel_x + ui->pos.x,
            rel_y + ui->pos.y);
}

SDL_Point core_project_ui(struct ui_core *ui, struct coord coord)
{
    int64_t rel_x = scale_div(ui->scale, coord.x - ui->pos.x);
    int64_t rel_y = scale_div(ui->scale, coord.y - ui->pos.y);
    return (SDL_Point) {
        .x = ui->rect.x + rel_x + ui->rect.w / 2,
        .y = ui->rect.y + rel_y + ui->rect.h / 2,
    };
}

void ui_core_render(struct ui_core *ui, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &ui->rect);
    
    struct rect rect = {
        .top = core_project_coord(ui, 0, 0),
        .bot = core_project_coord(ui, ui->rect.w, ui->rect.h),
    };

    SDL_SetTextureBlendMode(ui->tex, SDL_BLENDMODE_ADD);
        
    for (size_t i = 0; i < ui->sector->systems_len; ++i) {
        struct system *system = &ui->sector->systems[i];
        if (!rect_contains(&rect, system->coord)) continue;

        SDL_Point pos = core_project_ui(ui, system->coord);
        struct rgb rgb =
            desaturate(
                    spectrum_rgb(32 - bits_log2(system->star), 32),
                    .7);

        size_t px = scale_mult(ui->scale, 20);
        SDL_Rect src = (SDL_Rect) { .x = 0, .y = 0, .w = 100, .h = 100 };
        SDL_Rect dst = (SDL_Rect) {
            .x = pos.x - px / 2,
            .y = pos.y - px / 2,
            .h = px, .w = px
        };

        SDL_SetTextureColorMod(ui->tex, rgb.r, rgb.g, rgb.b);
        SDL_RenderCopy(renderer, ui->tex, &src, &dst);
    }
}

void ui_core_events(struct ui_core *ui, SDL_Event *event)
{
    (void) ui;
    (void) event;
}
