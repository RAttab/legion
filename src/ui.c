/* ui.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui.h"

#include "coord.h"
#include "sector.h"


struct ui_core
{
    struct sector *sector;
    SDL_Rect rect;

    uint32_t scale;
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
        .scale = 1,
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

void ui_core_render(struct ui_core *ui, SDL_Renderer *renderer)
{
    (void) renderer;
    
    for (size_t i = 0; i < ui->sector->systems_len; ++i) {
        struct system *system = &ui->sector->systems[i];
        (void) system;

        
    }
}

void ui_core_events(struct ui_core *ui, SDL_Event *event)
{
    (void) ui;
    (void) event;
}
