/* save.h
   Rémi Attab (remi.attab@gmail.com), 01 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

struct htable;
struct ring32;
struct ring64;
struct vec64;
struct symbol;

// -----------------------------------------------------------------------------
// magic
// -----------------------------------------------------------------------------

enum legion_packed save_magic
{
    save_magic_vec64 = 0x01,
    save_magic_ring32 = 0x02,
    save_magic_ring64 = 0x03,
    save_magic_htable = 0x04,
    save_magic_symbol = 0x05,
    save_magic_heap = 0x06,

    save_magic_core = 0x10,
    save_magic_world = 0x11,
    save_magic_sector = 0x12,
    save_magic_star = 0x13,

    save_magic_chunk = 0x20,
    save_magic_active = 0x21,
    save_magic_energy = 0x22,
    save_magic_log = 0x23,

    save_magic_mods = 0x30,
    save_magic_mod = 0x31,
    save_magic_atoms = 0x32,

    save_magic_lanes = 0x40,
    save_magic_lane = 0x41,

    save_magic_tape_set = 0x50,
};

static_assert(sizeof(enum save_magic) == 1);


// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

struct save;

struct save *save_new(const char *path, uint8_t version);
struct save *save_load(const char *path);
void save_close(struct save *);

bool save_eof(struct save *);
uint8_t save_version(struct save *);
size_t save_len(struct save *);

void save_write(struct save *, const void *src, size_t len);

#define save_write_value(save, _value)                  \
    do {                                                \
        typeof(_value) value = (_value);                \
        save_write(save, &value, sizeof(value));        \
    } while (false)

#define save_write_from(save, _ptr)                     \
    do {                                                \
        typeof(_ptr) ptr = (_ptr);                      \
        save_write(save, ptr, sizeof(*ptr));            \
    } while (false)

void save_read(struct save *, void *dst, size_t len);

#define save_read_type(save, type)                      \
    ({                                                  \
        type value;                                     \
        save_read(save, &value, sizeof(value));         \
        value;                                          \
    })

#define save_read_into(save, _ptr)                        \
    do {                                                  \
        typeof(_ptr) ptr = (_ptr);                        \
        save_read(save, ptr, sizeof(*ptr));               \
    } while (false)

void save_write_magic(struct save *, enum save_magic);
bool save_read_magic(struct save *, enum save_magic exp);

void save_write_htable(struct save *, const struct htable *);
bool save_read_htable(struct save *, struct htable *);

void save_write_vec64(struct save *, const struct vec64 *);
struct vec64 *save_read_vec64(struct save *);

void save_write_ring32(struct save *, const struct ring32 *);
struct ring32 *save_read_ring32(struct save *);

void save_write_ring64(struct save *, const struct ring64 *);
struct ring64 *save_read_ring64(struct save *);

void save_write_symbol(struct save *, const struct symbol *);
bool save_read_symbol(struct save *, struct symbol *);
