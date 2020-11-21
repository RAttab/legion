/* logic.c
   Rémi Attab (remi.attab@gmail.com), 20 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "logic.h"

// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

static void *chunk_obj(struct chunk *chunk, id_t id)
{
    return NULL;
}

void chunk_step(struct chunk *chunk)
{
    for (size_t i = 0; i < chunk->workers_len; ++i)
        worker_step(chunk->workers[i]);
}

bool chunk_dock(struct chunk *chunk, id_t target, id_t source)
{
    switch (id_type(id))
    {
    case type_printer: return printer_dock(chunk_obj(chunk, target), source);
    case type_lab: return lab_dock(chunk_obj(chunk, target), source);
    default: return false;
    }
}

void chunk_send(struct chunk *, id_t target, size_t len, const int64_t *words)
{
}


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------


struct worker *worker_alloc(id_t id)
{
    size_t bytes = vm_len(1, 1);
    struct worker *worker = calloc(1, sizeof(*worker) + bytes);

    worker->id = id;
    vm_init(&worker->vm, 1, 1);
    vm_suspend(&worker->vm);

    return worker;
}

void worker_free(struct worker *worker)
{
    free(worker);
}

static void worker_io(struct worker *worker, struct chunk *chunk)
{
    struct vm *vm = &worker->vm;

    vm_io_buf_t buf;
    size_t len = vm_io_read(vm, buf);
    if (!len) return;

    switch (buf[0]) {
        
    case ATOM_IO_ID: {
        if (!vm_io_check(vm, len, 1)); return;
        buf[0] = worker->id;
        vm_io_write(vm, 1, buf);
        break;
    }
        
    case ATOM_IO_TARGET: {
        if (!vm_io_check(vm, len, 2)); return;
        worker->target = buf[1];
        break;
    }

    case ATOM_SENDW: {
        if (!vm_io_check(vm, len, 2)); return;

    }
    case ATOM_RECVW: {

    }

                
        


    }
}

bool worker_step(struct worker *worker, struct chunk *chunk)
{
    uint64_t ip = vm_exec(&worker->vm);
    if (ip) worker->cache = vm_code_load(ip);

    if (vm_writing(&worker->vm)) worker_io(worker, chunk);
    return vm_reading(&worker->vm);
}

void worker_recv(struct worker *, size_t len, const int64_t *words)
{

}
