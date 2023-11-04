/* types.h
   Remi Attab (remi.attab@gmail.com), 04 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/types.h"
#include "db/items.h"
#include "game/coord.h"

// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------

typedef uint32_t world_ts;
typedef int64_t world_ts_delta;
typedef uint64_t world_seed;

struct legion_packed world_scan_it
{
    struct coord coord;
    uint64_t index;
};


// -----------------------------------------------------------------------------
// user
// -----------------------------------------------------------------------------

typedef uint8_t user_id;
typedef uint64_t user_set;


// -----------------------------------------------------------------------------
// im_id
// -----------------------------------------------------------------------------

typedef uint16_t im_id;
enum : size_t { im_id_shift = 8 };

inline im_id make_im_id(enum item type, im_id id) { return type << im_id_shift | id; }
inline enum item im_id_item(im_id id) { return id >> im_id_shift; }
inline uint32_t im_id_seq(im_id id) { return id & ((1 << im_id_shift) - 1); }

bool im_id_validate(vm_word word);

enum : size_t { im_id_str_len = item_str_len + 1 + 2 };
size_t im_id_str(im_id id, char *dst, size_t len);


// -----------------------------------------------------------------------------
// im misc
// -----------------------------------------------------------------------------

typedef uint8_t im_work;
typedef uint32_t im_energy;


// -----------------------------------------------------------------------------
// im_loops
// -----------------------------------------------------------------------------

typedef uint8_t im_loops;
enum : im_loops { im_loops_inf = UINT8_MAX };

inline im_loops im_loops_io(vm_word loops)
{
    return loops > 0 && loops < im_loops_inf ? loops : im_loops_inf;
}


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

typedef uint8_t tape_it;
typedef uint64_t tape_packed;

struct tape_set { uint64_t s[4]; };
static_assert(sizeof(struct tape_set) * 8 >= items_max);

enum tape_state : uint8_t
{
    tape_eof = 0,
    tape_input,
    tape_work,
    tape_output,
};


// -----------------------------------------------------------------------------
// cargo
// -----------------------------------------------------------------------------

struct legion_packed cargo
{
    enum item item;
    uint8_t count;
};
static_assert(sizeof(struct cargo) == 2);

inline struct cargo make_cargo(enum item item, uint8_t count)
{
    return (struct cargo) { .item = item, .count = count };
}

inline struct cargo cargo_nil()
{
    return make_cargo(item_nil, 0);
}

inline uint16_t cargo_to_u16(struct cargo cargo)
{
    return (((uint16_t) cargo.item) << 8) | cargo.count;
};

inline struct cargo cargo_from_u16(uint16_t data)
{
    return (struct cargo) { .item = data >> 8, .count = data & 0xFF, };
}

inline vm_word cargo_to_word(struct cargo cargo)
{
    return cargo_to_u16(cargo);
};

inline struct cargo cargo_from_word(vm_word word)
{
    assert(word >= 0 && word < UINT16_MAX);
    return cargo_from_u16(word);
};

inline int cargo_cmp(struct cargo lhs, struct cargo rhs)
{
    return lhs.item != rhs.item ? lhs.item - rhs.item : lhs.count - rhs.count;
}


// -----------------------------------------------------------------------------
// channels
// -----------------------------------------------------------------------------

enum : size_t { im_channels_max = 4 };

struct legion_packed im_channels
{
    im_id c[im_channels_max];
};
static_assert(sizeof(struct im_channels) == 8);

uint64_t im_channels_as_u64(struct im_channels channels);
struct im_channels im_channels_from_u64(uint64_t data);


// -----------------------------------------------------------------------------
// packet
// -----------------------------------------------------------------------------

enum : size_t { im_packet_max = 3 };

struct legion_packed im_packet
{
    uint8_t chan, len;
    legion_pad(6);
    vm_word data[im_packet_max];
};

static_assert(sizeof(struct im_packet) == 32);

vm_word im_packet_pack(uint8_t chan, uint8_t len);
void im_packet_unpack(vm_word word, uint8_t *chan, uint8_t *len);


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------
// Used in for ux_factory render and configured in im_config.gm.flow

typedef uint8_t im_rank;
constexpr im_rank im_rank_max = 16;
constexpr im_rank im_rank_sys = 14;

struct legion_packed flow
{
    im_id id;

    im_loops loops;
    enum item target;
    im_rank rank;

    enum item item;
    enum tape_state state;
    tape_it tape_it, tape_len;

    legion_pad(1);
};

static_assert(sizeof(struct flow) == 10);


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

enum : size_t { man_path_max = 64, man_toc_max = 16 };

typedef uint16_t man_line;
typedef uint16_t man_page;
typedef uint16_t man_section;

struct legion_packed link { man_page page; man_section section; };

// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------

enum status_type : uint8_t
{
    st_info = 0,
    st_warn = 1,
    st_error = 2,
};
static_assert(sizeof(enum status_type) == 1);
