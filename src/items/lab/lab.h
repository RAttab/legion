/* lab.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "utils/rng.h"

struct im_config;


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

enum legion_packed im_lab_state
{
    im_lab_idle = 0,
    im_lab_waiting = 1,
    im_lab_working = 2,
};

struct legion_packed im_lab
{
    id_t id;

    enum item item;
    enum im_lab_state state;
    struct { uint8_t left; uint8_t cap; } work;

    struct rng rng;
};

static_assert(sizeof(struct im_lab) == 16);

void im_lab_config(struct im_config *);


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

struct im_lab_bits
{
    uint8_t bits;
    uint64_t known;
    struct dim margin;
    struct font *font;
};

struct im_lab_bits im_lab_bits_new(struct font *);
void im_lab_bits_update(struct im_lab_bits *, struct world *, enum item);
void im_lab_bits_render(struct im_lab_bits *, struct ui_layout *, SDL_Renderer *);
