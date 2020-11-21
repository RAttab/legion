/* logic.h
   Rémi Attab (remi.attab@gmail.com), 20 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


struct cargo { uint16_t type, count; };

// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

typedef uint32_t id_t;
typedef uint8_t type_t;

enum types
{
    type_worker,
    type_lab,
    type_printer,
};

inline type_t id_type(id_t id) { return id >> (32 - 8); }
inline id_t make_id(id_t id, type_t type) { return id | type << (32 - 8); }


// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

struct chunk
{
    struct htable index;

    size_t workers_len;
    struct workers *workers;

    size_t broadcast_len;
    int64_t broadcast[16];
};

id_t chunk_id(struct chunk *);


void chunk_step(struct chunk *);
bool chunk_dock(struct chunk *, id_t target, id_t source);
void chunk_send(struct chunk *, id_t target, size_t len, const int64_t *words);
void chunk_broadcast(struct chunk *, size_t len, const int64_t *words);


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------


struct worker
{
    id_t id;
    id_t target;
    id_t dock;
    struct vm_code *cache;
    struct cargo slots[2];
    struct vm vm;
};

struct worker *worker_alloc(struct chunk *chunk, uint64_t ip);
void worker_free(struct worker *);

void worker_step(struct worker *);
void worker_recv(struct worker *, size_t len, const int64_t *words);


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

struct printer
{
    id_t id;
    id_t target;
    id_t docks[2];
    
    uint8_t slots_len;
    struct cargo *slots;

    struct vm *vm;
};

// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

struct lab
{
    id_t id;
    id_t target;
    id_t docks[2];

    uint8_t slots_len;
    struct cargo *slots;

    struct vm *vm;
};
