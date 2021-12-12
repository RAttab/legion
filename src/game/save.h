/* save.h
   Rémi Attab (remi.attab@gmail.com), 01 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

struct htable;
struct vec64;
struct symbol;

// -----------------------------------------------------------------------------
// magic
// -----------------------------------------------------------------------------

enum legion_packed save_magic
{
    save_magic_vec64  = 0x01,
    save_magic_ring32 = 0x02,
    save_magic_ring64 = 0x03,
    save_magic_htable = 0x04,
    save_magic_symbol = 0x05,
    save_magic_heap   = 0x06,

    save_magic_sim      = 0x10,
    save_magic_world    = 0x11,
    save_magic_star     = 0x12,
    save_magic_lab      = 0x13,
    save_magic_tech     = 0x14,
    save_magic_log      = 0x15,
    save_magic_lanes    = 0x16,
    save_magic_lane     = 0x17,
    save_magic_tape_set = 0x18,

    save_magic_atoms   = 0x20,
    save_magic_mods    = 0x21,
    save_magic_mod     = 0x22,
    save_magic_chunks  = 0x28,
    save_magic_chunk   = 0x29,
    save_magic_active  = 0x2A,
    save_magic_energy  = 0x2B,

    save_magic_state_world   = 0x30,
    save_magic_state_compile = 0x31,
    save_magic_state_mod     = 0x32,
    save_magic_state_chunk   = 0x33,

    save_magic_len,
};

static_assert(sizeof(enum save_magic) == 1);


// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

struct save;

bool save_eof(struct save *);
size_t save_cap(struct save *);
size_t save_len(struct save *);
uint8_t *save_bytes(struct save *);

// -----------------------------------------------------------------------------
// save_mem
// -----------------------------------------------------------------------------

struct save *save_mem_new(void);
void save_mem_free(struct save *);

void save_mem_reset(struct save *);


// -----------------------------------------------------------------------------
// save_ring
// -----------------------------------------------------------------------------

struct save_ring;

struct save_ring *save_ring_new(size_t cap);
void save_ring_free(struct save_ring *);

void save_ring_close(struct save_ring *);
bool save_ring_closed(struct save_ring *);

void save_ring_clear_reads(struct save_ring *);
void save_ring_clear_writes(struct save_ring *);

struct save *save_ring_read(struct save_ring *);
struct save *save_ring_write(struct save_ring *);
void save_ring_commit(struct save_ring *, struct save *);

size_t save_ring_consume(struct save *, size_t len);

int save_ring_wake_fd(struct save_ring *);
void save_ring_wake_signal(struct save_ring *);
void save_ring_wake_drain(struct save_ring *);


// -----------------------------------------------------------------------------
// save_file
// -----------------------------------------------------------------------------

struct save *save_file_create(const char *path, uint8_t version);
struct save *save_file_load(const char *path);
void save_file_close(struct save *);
uint8_t save_file_version(struct save *);


// -----------------------------------------------------------------------------
// read/write
// -----------------------------------------------------------------------------

size_t save_write(struct save *, const void *src, size_t len);

#define save_write_value(save, _value)                          \
    do {                                                        \
        typeof(_value) value = (_value);                        \
        size_t ret = save_write(save, &value, sizeof(value));   \
        assert(ret == sizeof(value));                           \
    } while (false)

#define save_write_from(save, _ptr)                             \
    do {                                                        \
        typeof(_ptr) ptr = (_ptr);                              \
        size_t ret = save_write(save, ptr, sizeof(*ptr));       \
        assert(ret == sizeof(*ptr));                            \
    } while (false)

size_t save_read(struct save *, void *dst, size_t len);

#define save_read_type(save, type)                              \
    ({                                                          \
        type value;                                             \
        size_t ret = save_read(save, &value, sizeof(value));    \
        assert(ret == sizeof(value));                           \
        value;                                                  \
    })

#define save_read_into(save, _ptr)                              \
    do {                                                        \
        typeof(_ptr) ptr = (_ptr);                              \
        size_t ret = save_read(save, ptr, sizeof(*ptr));        \
        assert(ret == sizeof(*ptr));                            \
    } while (false)

void save_write_magic(struct save *, enum save_magic);
bool save_read_magic(struct save *, enum save_magic exp);

void save_write_htable(struct save *, const struct htable *);
bool save_read_htable(struct save *, struct htable *);

void save_write_vec64(struct save *, const struct vec64 *);
bool save_read_vec64(struct save *, struct vec64 **);

void save_write_symbol(struct save *, const struct symbol *);
bool save_read_symbol(struct save *, struct symbol *);

void save_prof(struct save *);
void save_prof_dump(struct save *);
