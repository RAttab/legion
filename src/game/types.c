/* types.c
   Remi Attab (remi.attab@gmail.com), 04 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "types.h"

// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

bool im_id_validate(vm_word word)
{
    return
        word > 0 &&
        word < UINT16_MAX &&
        item_validate(im_id_item(word));
}

size_t im_id_str(im_id id, char *base, size_t len)
{
    assert(len >= im_id_str_len);
    char *it = base;
    const char *end = base + len;

    it += item_str(im_id_item(id), it, end - it);
    *it = '.'; it++;
    it += str_utox(im_id_seq(id), it, 2);

    if (it != end) *it = 0;
    return it - base;
}


// -----------------------------------------------------------------------------
// channel
// -----------------------------------------------------------------------------

uint64_t im_channels_as_u64(struct im_channels channels)
{
    const size_t bits = 16;
    return
        ((uint64_t) channels.c[0]) << (0 * bits) |
        ((uint64_t) channels.c[1]) << (1 * bits) |
        ((uint64_t) channels.c[2]) << (2 * bits) |
        ((uint64_t) channels.c[3]) << (3 * bits);
}

struct im_channels im_channels_from_u64(uint64_t data)
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

vm_word im_packet_pack(uint8_t chan, uint8_t len)
{
    assert(chan < im_channels_max);
    assert(len <= im_packet_max);
    return vm_pack(chan, len);
}

void im_packet_unpack(vm_word word, uint8_t *chan, uint8_t *len)
{
    uint32_t chan32 = 0, len32 = 0;
    vm_unpack(word, &chan32, &len32);

    assert(chan32 < im_channels_max);
    *chan = chan32;

    assert(len32 <= im_packet_max);
    *len = len32;
}
