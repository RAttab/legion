/* obj.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

enum otype
{
    OBJ_WORKER,
};

// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

typedef uint32_t id_t;

inline id_t make_id(enum otype type, id_t id) { return type << 24 | id; }
inline enum otype id_type(id_t id) { return id >> 24; }


// -----------------------------------------------------------------------------
// cargo
// -----------------------------------------------------------------------------

typedef uint8_t item_t;
typedef uint16_t cargo_t;

inline cargo_t make_cargo(item_t item, uint8_t count) { return (((cargo_t)item) << 8) | count; }
inline item_t cargo_item(cargo_t cargo) { return cargo >> 8; }
inline item_t cargo_count(cargo_t cargo) { return (uint8_t) cargo; }


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

    struct vm_code *code;

    uint8_t type;
    struct { uint8_t len:4; uint8_t cap:4; } io;
    uint8_t docks;
    uint8_t cargos;

    uint8_t off_docks;
    uint8_t off_cargo;
    uint8_t off_vm;
    uint8_t off_state;
}; // u64 * 4;

struct obj *obj_alloc(id_t id, const struct obj_spec *);
void obj_free(struct obj *);

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
