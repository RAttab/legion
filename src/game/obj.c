/* obj.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "obj.h"

#include "vm/vm.h"
#include "vm/mod.h"
#include "game/atoms.h"
#include "game/hunk.h"

// -----------------------------------------------------------------------------
// obj
// -----------------------------------------------------------------------------

struct obj *obj_alloc(struct hunk *hunk, item_t type, const struct obj_spec *spec)
{
    size_t len_io = spec->io * sizeof(word_t);
    size_t len_docks = spec->docks * sizeof(id_t);
    size_t len_cargo = spec->cargo * sizeof(cargo_t);
    size_t len_state = spec->state;
    size_t len_head = align_cache(
            sizeof(struct obj) + len_io + len_docks + len_cargo + len_state);
    size_t len_vm = vm_len(spec->stack);
    size_t len_total = len_head + len_vm;
    assert(len_total <= 0xFF);

    struct obj *obj = hunk_obj_alloc(hunk, type, len_total);
    obj->len = len_total;
    obj->io.len = 0;
    obj->io.cap = spec->io;
    obj->cargos = spec->cargo;
    obj->docks = spec->docks;

    obj->off_docks = sizeof(*obj) + len_io;
    obj->off_cargo = obj->off_docks + len_docks;
    obj->off_state = obj->off_cargo + len_cargo;
    obj->off_vm = len_head;

    return obj;
}


static void obj_process_io(struct obj *obj, struct hunk *hunk)
{
    struct vm *vm = obj_vm(obj);
    
    vm_io_buf_t buf;
    size_t len = vm_io_read(vm, buf);
    if (!len) return;

    switch (buf[0]) {
    case IO_NOOP: { return; }

    case IO_ID: {
        if (!vm_io_check(vm, len, 1)) return;
        buf[0] = obj->id;
        vm_io_write(vm, 1, buf);
        return;
    }

    case IO_TARGET: {
        if (!vm_io_check(vm, len, 2)) return;
        obj->target = buf[1];
        return;
    }

    case IO_SEND: {
        if (!vm_io_check(vm, len, 2)) return;

        struct obj *target = hunk_obj(hunk, obj->target);
        if (!target || target->io.cap < 1) return;

        target->io.len = 1;
        obj_io(target)[0] = buf[1];
        return;
    }

    case IO_SENDN: {
        struct obj *target = hunk_obj(hunk, obj->target);
        if (!target) return;

        target->io.len = i64_min(len, target->io.cap);
        memcpy(obj_io(target), buf, i64_min(len, target->io.len));
        return;
    }

    case IO_RECV: {
        buf[0] = obj_io(obj)[0];
        vm_io_write(vm, !!obj->io.len, buf);
        return;
    }

    case IO_RECVN: {
        memcpy(buf, obj_io(obj), obj->io.len);
        vm_io_write(vm, obj->io.len, buf);
        return;
    }

    case IO_CARGO: {
        if (!vm_io_check(vm, len, 2)) return;

        size_t slot = buf[1];
        if (slot >= obj->cargos) { buf[0] = 0; }
        else {
            cargo_t cargo = obj_cargo(obj)[slot];
            buf[0] = vm_pack(cargo_item(cargo), cargo_count(cargo));
        }

        vm_io_write(vm, 1, buf);
        return;
    }

    case IO_VENT: {
        if (!vm_io_check(vm, len, 2)) return;
        size_t slot = buf[1];
        if (slot < obj->cargos)
            obj_cargo(obj)[slot] = 0;
        return;
    }

    }

    void *state = obj_state(obj);
    bool consumed = false;

    switch (id_item(obj->id)) {

    case ITEM_WORKER: { consumed = worker_io(obj, hunk, state, buf, len); break; }

    default: assert(false && "unknown object type");

    }

    if (!consumed) vm_io_fault(vm);
}

void obj_load(struct obj *obj, mod_t id)
{
    struct vm *vm = obj_vm(obj);
    vm_reset(vm);

    mod_discard(obj->mod);
    obj->mod = mods_load(id);
}

void obj_step(struct obj *obj, struct hunk *hunk)
{
    if (!obj->mod) return;

    struct vm *vm = obj_vm(obj);
    ip_t ip = vm_exec(vm,  obj->mod);
    if (!ip) {
        mod_discard(obj->mod);
        obj->mod = mods_load(ip_mod(ip));
    }

    if (vm_io(vm)) obj_process_io(obj, hunk);
}
