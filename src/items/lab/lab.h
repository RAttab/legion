/* lab.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "utils/rng.h"
#include "ui/ui.h"

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
    im_id id;

    enum item item;
    enum im_lab_state state;
    struct { im_work left, cap; } work;

    legion_pad(2);

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
};

struct im_lab_bits im_lab_bits_new(void);
void im_lab_bits_update(struct im_lab_bits *, const struct tech *, enum item);
void im_lab_bits_render(struct im_lab_bits *, struct ui_layout *, SDL_Renderer *);
