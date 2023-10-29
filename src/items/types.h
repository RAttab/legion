/* types.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/tape.h"
#include "db/items.h"
#include "vm/vm.h"


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

typedef uint8_t im_loops;
enum : im_loops { im_loops_inf = UINT8_MAX };

inline im_loops im_loops_io(vm_word loops)
{
    return loops > 0 && loops < im_loops_inf ? loops : im_loops_inf;
}


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
    return (struct cargo) {
        .item = data >> 8,
        .count = data & 0xFF,
    };
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
// flow
// -----------------------------------------------------------------------------

// used in config->gm.flow

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
// channels
// -----------------------------------------------------------------------------

enum : size_t { im_channels_max = 4 };

struct legion_packed im_channels
{
    im_id c[im_channels_max];
};
static_assert(sizeof(struct im_channels) == 8);

inline uint64_t im_channels_as_u64(struct im_channels channels)
{
    const size_t bits = 16;
    return
        ((uint64_t) channels.c[0]) << (0 * bits) |
        ((uint64_t) channels.c[1]) << (1 * bits) |
        ((uint64_t) channels.c[2]) << (2 * bits) |
        ((uint64_t) channels.c[3]) << (3 * bits);
}

inline struct im_channels im_channels_from_u64(uint64_t data)
{
    const size_t bits = 16;
    const uint64_t mask = (1ULL << bits) - 1;
    return (struct im_channels) {
        .c = {
            [0] = (data >> (0 * bits)) & mask,
            [1] = (data >> (1 * bits)) & mask,
            [2] = (data >> (2 * bits)) & mask,
            [3] = (data >> (3 * bits)) & mask,
        }
    };
}


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

inline vm_word im_packet_pack(uint8_t chan, uint8_t len)
{
    assert(chan < im_channels_max);
    assert(len <= im_packet_max);
    return vm_pack(chan, len);
}

inline void im_packet_unpack(vm_word word, uint8_t *chan, uint8_t *len)
{
    uint32_t chan32 = 0, len32 = 0;
    vm_unpack(word, &chan32, &len32);

    assert(chan32 < im_channels_max);
    *chan = chan32;

    assert(len32 <= im_packet_max);
    *len = len32;
}
