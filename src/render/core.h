/* core.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "SDL.h"

struct map;
struct panel;
struct sector;


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

enum event
{
    EV_NIL = 0,

    EV_MODS_SELECT,
    EV_MODS_UPDATE,
    EV_MODS_CLEAR,

    EV_CODE_SELECT,
    EV_CODE_CLEAR,

    EV_STAR_SELECT,
    EV_STAR_UPDATE,
    EV_STAR_CLEAR,

    EV_OBJ_SELECT,
    EV_OBJ_CLEAR,

    EV_MAX,
};


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct core
{
    SDL_Rect rect;
    SDL_Window *window;
    SDL_Renderer *renderer;

    uint32_t event;
    uint64_t ticks;

    struct {
        SDL_Point point;
        SDL_Texture *tex;
        size_t size;
    } cursor;

    struct {
        struct map *map;
        struct panel *menu;
        struct panel *mods;
        struct panel *code;
        struct panel *pos;
        struct panel *star;
    } ui;

    struct {
        struct sector *sector;
    } state;
};

extern struct core core;

void core_init();
void core_close();

void core_path_res(const char *name, char *dst, size_t len);

void core_run();
void core_quit();

void core_push_event(enum event, void *data);
