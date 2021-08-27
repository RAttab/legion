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

typedef uint16_t loops_t;
enum { loops_inf = UINT16_MAX };
inline loops_t loops_io(word_t loops)
{
    return loops > 0 && loops < loops_inf ? loops : loops_inf;
}


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

// used in config->gm.flow

struct legion_packed flow
{
    id_t id;
    uint16_t row, col;

    loops_t loops;
    enum item target;
    uint8_t rank;

    enum item in, out;
    tape_it_t tape_it, tape_len;
};

static_assert(sizeof(struct flow) == 16);


// -----------------------------------------------------------------------------
// packet
// -----------------------------------------------------------------------------

enum
{
    im_packet_max = 3,
    im_channel_max = 4,
};

struct legion_packed im_packet
{
    uint8_t chan, len;
    legion_pad(6);
    word_t data[im_packet_max];
};

static_assert(sizeof(struct im_packet) == 32);

inline word_t im_packet_pack(uint8_t chan, uint8_t len)
{
    assert(chan < im_channel_max);
    assert(len <= im_packet_max);
    return vm_pack(chan, len);
}

inline void im_packet_unpack(word_t word, uint8_t *chan, uint8_t *len)
{
    uint32_t chan32 = 0, len32 = 0;
    vm_unpack(word, &chan32, &len32);

    assert(chan32 < im_channel_max);
    *chan = chan32;

    assert(len32 <= im_packet_max);
    *len = len32;
}
