/* nomad.h
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// nomad
// -----------------------------------------------------------------------------

enum legion_packed im_nomad_op
{
    im_nomad_nil = 0,
    im_nomad_pack,
    im_nomad_load,
    im_nomad_unload,
};

static_assert(sizeof(enum im_nomad_op) == 1);


struct legion_packed im_nomad_cargo
{
    enum item item;
    uint8_t count;
};

static_assert(sizeof(struct im_nomad_cargo) == 2);

enum
{
    im_nomad_data_len = 3,
    im_nomad_cargo_len = 12,
    im_nomad_cargo_max = UINT8_MAX,
};


struct legion_packed im_nomad
{
    id_t id;

    mod_t mod;

    enum im_nomad_op op;
    enum item item;
    loops_t loops;
    bool waiting;

    legion_pad(3);

    struct im_nomad_cargo cargo[im_nomad_cargo_len];
    word_t data[im_nomad_data_len];
};

static_assert(sizeof(struct im_nomad) == 64);


void im_nomad_config(struct im_config *);
