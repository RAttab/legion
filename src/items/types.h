/* types.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/tape.h"
#include "items/item.h"
#include "vm/vm.h"


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

typedef uint8_t loops;
enum { loops_inf = UINT8_MAX };
inline loops loops_io(vm_word loops)
{
    return loops > 0 && loops < loops_inf ? loops : loops_inf;
}


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

// used in config->gm.flow

struct legion_packed flow
{
    id id;
    uint16_t row, col;

    legion_pad(2);

    loops loops;
    enum item target;
    uint8_t rank;

    legion_pad(1);

    enum item in, out;
    tape_it tape_it, tape_len;
};

static_assert(sizeof(struct flow) == 16);


// -----------------------------------------------------------------------------
// channels
// -----------------------------------------------------------------------------

enum { im_channels_max = 4 };

struct legion_packed im_channels
{
    id c[im_channels_max];
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

enum { im_packet_max = 3 };

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
