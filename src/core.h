/* core.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "utils.h"

// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

enum event
{
    EV_SYSTEM_SELECT = 0,
    EV_SYSTEM_CLEAR,

    EV_MAX,
};

// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct sector;
struct map;
struct panel;

struct core
{
    SDL_Rect rect;
    SDL_Window *window;
    SDL_Renderer *renderer;

    uint32_t event;

    struct {
        SDL_Point point;
        SDL_Texture *tex;
        size_t size;
    } cursor;

    struct {
        struct map *map;
        struct panel *pos;
        struct panel *system;
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