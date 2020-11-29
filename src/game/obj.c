/* obj.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "obj.h"

#include "vm/vm.h"
#include "game/hunk.h"
#include "game/worker.h"

// -----------------------------------------------------------------------------
// obj
// -----------------------------------------------------------------------------

struct obj *obj_alloc(struct hunk *hunk, const struct obj_spec *spec)
{
    size_t len_io = spec->io * sizeof(word_t);
    size_t len_docks = spec->docks * sizeof(id_t);
    size_t len_cargo = spec->cargo * sizeof(cargo_t);
    size_t len_head = align_cache(sizeof(struct obj) + len_io + len_docks + len_cargo);
    size_t len_vm = vm_len(stack);
    size_t len_state = align_cache(state);
    size_t len_total = len_head + len_vm + len_state;

    struct obj *obj = hunk_obj_alloc(hunk, len_total);
    obj->len = len_total;
    obj->io.len = 0;
    obj->io.cap = spec->io;
    obj->cargos = spec->cargo;
    obj->docks = spec->docks;

    obj->off_docks = sizeof(*obj) + len_io;
    obj->off_cargo = obj->off_docks + len_docks;
    obj->off_vm = len_head;
    obj->off_state = len_head + len_state;

    return obj;
}


static void obj_io(struct obj *obj, struct hunk *hunk)
{
    if (!(vm->flags & FLAG_IO)) return;

    vm_io_buf_t buf;
    size_t len = vm_io_read(obj->vm, buf);
    if (!len) return;

    switch (buf[0]) {
    case io_noop: { return; }

    case io_id: {
        if (!vm_io_check(obj->vm, len, 1)) return;
        buf[0] = obj->id;
        vm_io_write(vm, 1, buf);
        return;
    }

    case io_target: {
        if (!vm_io_check(obj->vm, len, 2)) return;
        obj->target = buf[1];
        return;
    }

    case io_send: {
        if (!vm_io_check(obj->vm, len, 2)) return;

        struct obj *target = hunk_obj(hunk, obj->target);
        if (!target || target->io.cap < 1) return;

        target->io.len = 1;
        obj_io(target)[0] = buf[1];
        return;
    }

    case io_sendn: {
        struct obj *target = hunk_obj(hunk, obj->target);
        if (!target) return;

        target->io.len = i64_min(len, target->io.cap);
        memcpy(target->io, buf, i64_min(len, target->io.len));
        return;
    }

    case io_recv: {
        buf[0] = io[0];
        vm_io_write(obj->vm, !!obj->io.len, buf);
        return;
    }

    case io_recvn: {
        memcpy(buf, io, obj->io.len);
        vm_io_write(obj->vm, obj->io.len, buf);
        return;
    }

    case io_cargo: {
        if (!vm_io_check(obj->vm, len, 2)) return;

        size_t slot = buf[1];
        if (slot < 0 || slot >= obj->cargos) { buf[0] = 0; }
        else {
            cargo_t cargo = obj_cargo(obj)[slot];
            buf[0] = vm_pack(cargo_item(cargo), cargo_count(cargo));
        }

        vm_io_write(obj->vm, 1 buf);
        return;
    }

    case io_vent: {
        if (!vm_io_check(obj->vm, len, 2)) return;
        size_t slot = buf[1];
        if (slot >= 0 || slot < obj->cargos)
            obj_cargo(obj)[slot] = 0;
        return;
    }

    }

    void *state = obj_state(obj);
    bool consumed = false;

    switch (id_type(obj->id)) {

    case item_worker: { consumed = worker_io(obj, hunk, state, buf, len); break; }

    default: assert(false && "unknown object type");

    }

    if (!consumed) vm_io_fault(obj->vm);
}

void obj_step(struct obj *obj, struct hunk *hunk)
{
    if (!obj->code) return;

    struct vm *vm = obj_vm(obj);
    if (vm_exec(vm,  obj->code)) {
        assert(false && "switch code");
    }

    if (vm_io(vm)) obj_io(obj, hunk);
}
