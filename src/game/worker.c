/* worker.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "worker.h"
#include "game/obj.h"
#include "game/hunk.h"

// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

enum
{
    worker_io = 2,
    worker_cargo = 2,
};

struct obj *worker_alloc(id_t id)
{
    return obj_alloc(id, obj_worker, &(struct obj_spec) {
                .state = sizeof(worker),
                .stack = 0,
                .io = worker_io,
                .cargo = worker_cargo,
                .docks = 0
            });
}

static bool worker_dock(
        struct worker *state, struct obj *obj, struct hunk *hunk, id_t id)
{
    if (state->dock) return false;

    struct obj *target = hunk_obj(hunk, buf[1]);
    if (!target) return false;

    id_t *docks = obj_docks(target);
    for (size_t i = 0; i < target->docks; ++i) {
        if (docks[i]) continue;
        docks[i] = obj->id;
        state->dock = target->id;
        return true;
    }

    return false;
}

static void worker_undock(
        struct worker *state, struct obj *obj, struct hunk *hunk)
{
    if (!state->dock) return;

    struct obj *target = hunk_obj(hunk, state->dock);
    assert(target && "docked to unknown id");

    id_t *docks = obj_docks(target);
    for (size_t i = 0; i < target->docks; ++i) {
        if (docks[i] != obj->id) continue;

        docks[i] = 0;
        state->dock = 0;
        return;
    }
}

static cargo_t *worker_slot_local(struct obj *obj, size_t slot)
{
    cargo_t *cargo = obj_cargo(obj);
    if (slot) return slot < obj->cargos ? cargo[slot] : NULL;

    cargo_t *end = cargo + obj->cargos;
    while (cargo != end) if (!*cargo) return cargo;

    return obj_cargo(obj);
}

static cargo_t *worker_slot_remote(struct obj *obj, size_t type)
{
    cargo_t *cargo = obj_cargo(obj);
    cargo_t *end = cargo + obj->cargos;
    while (cargo != end) {
        item_t item = cargo_item(*cargo);
        if (item && (!type || type == item)) return cargo;
    }

    return obj_cargo(obj);
}

static void worker_take(
        struct worker *state, struct obj *obj, struct hunk *hunk, word_t arg)
{
    if (!state->dock) return;

    uint32_t type, slot;
    vm_unpack(arg, &type, &slot);

    cargo_t *dst = worker_slot_local(obj, slot);
    if (!src) return;

    struct obj *target = hunk_obj(hunk, state->dock);
    assert(target && "docked to unknown id");

    cargo_t *src = worker_slot_remote(target, type);
    if (!dst && *dst) return;

    *dst = *src;
    *src = 0;
}

static void worker_put(
        struct worker *state, struct obj *obj, struct hunk *hunk, word_t arg)
{
    if (!state->dock) return;

    uint32_t type, slot;
    vm_unpack(arg, &type, &slot);

    cargo_t *src = worker_slot_local(obj, slot);
    if (!src || !*src) return;

    struct obj *target = hunk_obj(hunk, state->dock);
    assert(target && "docked to unknown id");

    cargo_t *dst = worker_slot_remote(target, type);
    if (!dst) return;

    *dst = *src;
    *src = 0;
}

static void worker_harvest(struct obj *obj, struct hunk *hunk, word_t arg)
{
    size_t count = hunk_harvest(hunk, buf[1], 1);
    if (!count) return;

    cargo_t *cargo = obj_cargo(obj);
    cargo_t *end = cargo + obj->cargos;

    while (cargo != end) {
        item_t item = cargo_item(*cargo);
        if (!item) { *cargo = make_cargo(arg, 1); return; }
        if (item == arg) { *cargo = cargo_inc(*cargo); return; }
    }

    return;
}


bool worker_io(
        struct obj *obj,
        struct hunk *hunk,
        void *state_,
        int64_t *buf,
        size_t len)
{
    struct worker *state = state_;

    switch (buf[0]) {

    case io_dock: {
        if (!vm_io_check(obj->vm, len, 2)) return true;
        buf[0] = worker_dock(state, obj, hunk, buf[1]) ? io_ok : io_fail;
        vm_io_write(obj->vm, 1, buf);
        return true;
    }

    case io_undock: {
        if (!vm_io_check(obj->vm, len, 1)) return true;
        worker_undock(state, obj, hunk);
        return true;
    }

    case io_take: {
        if (!vm_io_check(obj->vm, len, 2)) return true;
        worker_take(state, obj, hunk, buf[1]);
        return true;
    }

    case io_put: {
        if (!vm_io_check(obj->vm, len, 2)) return true;
        worker_put(state, obj, hunk, buf[1]);
        return true;
    }

    case io_harvest: {
        if (!vm_io_check(obj->vm, len, 2)) return true;
        worker_harvest(obj, hunk, buf[1]);
        return true;
    }

    default: return false;
    }
}
