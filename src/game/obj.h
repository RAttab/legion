/* obj.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "vm/vm.h"

struct hunk;

// -----------------------------------------------------------------------------
// obj_spec
// -----------------------------------------------------------------------------

struct obj_spec
{
    uint8_t state;
    uint8_t stack;
    uint8_t io;
    uint8_t cargo;
    uint8_t docks;
};


// -----------------------------------------------------------------------------
// obj
// -----------------------------------------------------------------------------

struct legion_packed obj
{
    id_t id;
    id_t target;

    struct mod *mod;

    uint8_t len;
    struct { uint8_t len:4; uint8_t cap:4; } io;
    uint8_t docks;
    uint8_t cargos;

    uint8_t off_docks;
    uint8_t off_cargo;
    uint8_t off_vm;
    uint8_t off_state;
}; // u64 * 4;

struct obj *obj_alloc(struct hunk *, item_t type, const struct obj_spec *);

void obj_load(struct obj *, mod_t);
void obj_step(struct obj *, struct hunk *);

inline word_t *obj_io(struct obj *obj)
{
    return ((void *) obj) + sizeof(*obj);
}

inline id_t *obj_docks(struct obj *obj)
{
    return ((void *) obj) + obj->off_docks;
}

inline cargo_t *obj_cargo(struct obj *obj)
{
    return ((void *) obj) + obj->off_cargo;
}

inline struct vm *obj_vm(struct obj *obj)
{
    return ((void *) obj) + obj->off_vm;
}

inline void *obj_state(struct obj *obj)
{
    return ((void *) obj) + obj->off_state;
}


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

struct legion_packed worker { id_t dock; };
struct obj *worker_alloc(struct hunk *);
bool worker_io(struct obj *, struct hunk *, void *state, int64_t *buf, size_t len);
