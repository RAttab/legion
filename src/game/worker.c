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
    worker_spec_io = 2,
    worker_spec_cargo = 2,
};

struct obj *worker_alloc(struct hunk * hunk)
{
    return obj_alloc(hunk, ITEM_WORKER, &(struct obj_spec) {
                .state = sizeof(struct worker),
                .stack = 0,
                .io = worker_spec_io,
                .cargo = worker_spec_cargo,
                .docks = 0
            });
}

static bool worker_dock(
        struct worker *state, struct obj *obj, struct hunk *hunk, id_t id)
{
    if (state->dock) return false;

    struct obj *target = hunk_obj(hunk, id);
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
    if (slot) return slot < obj->cargos ? &cargo[slot] : NULL;

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
    if (!dst) return;

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

static void worker_harvest(struct obj *obj, struct hunk *hunk, word_t ele)
{
    if (ele < elem_natural_first || ele > elem_natural_last) return;
    
    size_t count = hunk_harvest(hunk, ele, 1);
    if (!count) return;

    cargo_t *cargo = obj_cargo(obj);
    cargo_t *end = cargo + obj->cargos;

    while (cargo != end) {
        item_t item = cargo_item(*cargo);
        if (!item) { *cargo = make_cargo(ele, 1); return; }
        if (item == ele) { *cargo = cargo_add(*cargo, 1); return; }
    }
}


bool worker_io(
        struct obj *obj,
        struct hunk *hunk,
        void *state_,
        int64_t *buf,
        size_t len)
{
    struct worker *state = state_;
    struct vm *vm = obj_vm(obj);

    switch (buf[0]) {

    case IO_DOCK: {
        if (!vm_io_check(vm, len, 2)) return true;
        buf[0] = worker_dock(state, obj, hunk, buf[1]) ? IO_OK : IO_FAIL;
        vm_io_write(vm, 1, buf);
        return true;
    }

    case IO_UNDOCK: {
        if (!vm_io_check(vm, len, 1)) return true;
        worker_undock(state, obj, hunk);
        return true;
    }

    case IO_TAKE: {
        if (!vm_io_check(vm, len, 2)) return true;
        worker_take(state, obj, hunk, buf[1]);
        return true;
    }

    case IO_PUT: {
        if (!vm_io_check(vm, len, 2)) return true;
        worker_put(state, obj, hunk, buf[1]);
        return true;
    }

    case IO_HARVEST: {
        if (!vm_io_check(vm, len, 2)) return true;
        worker_harvest(obj, hunk, buf[1]);
        return true;
    }

    default: return false;
    }
}
