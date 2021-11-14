/* txrx.c
   Rémi Attab (remi.attab@gmail.com), 28 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/transmit/transmit.h"


// -----------------------------------------------------------------------------
// test
// -----------------------------------------------------------------------------

void test_txrx(void)
{
    struct world *world = world_new(0);
    const struct sector *sector = world_sector(world, coord_center());
    const struct coord src = sector->stars[0].coord;
    const struct coord dst = sector->stars[1].coord;
    struct chunk *chunk_src = world_chunk_alloc(world, src);
    struct chunk *chunk_dst = world_chunk_alloc(world, dst);

    chunk_create(chunk_src, ITEM_TRANSMIT);
    chunk_create(chunk_dst, ITEM_RECEIVE);
    chunk_create(chunk_dst, ITEM_TEST);
    world_step(world);

    id_t id_tx = make_id(ITEM_TRANSMIT, 1);
    id_t id_rx = make_id(ITEM_RECEIVE, 1);
    id_t id_test = make_id(ITEM_TEST, 1);

    const struct im_test *test = chunk_get(chunk_dst, id_test);
    const word_t packet[im_packet_max] = {
        0x1111111111111111,
        0x2222222222222222,
        0x3333333333333333
    };

    // nil rx
    chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
    assert(im_test_check(test, IO_RECV, id_rx, NULL, 0));

    { // unconfigured
        chunk_io(chunk_src, IO_TRANSMIT, 0, id_tx, packet, array_len(packet));
        wait(world, im_transmit_speed, src, dst);
        chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, IO_RECV, id_rx, NULL, 0));
    }

    { // configure - tx
        word_t target = coord_to_u64(dst);
        chunk_io(chunk_src, IO_TARGET, 0, id_tx, &target, 1);

        chunk_io(chunk_src, IO_TRANSMIT, 0, id_tx, packet, array_len(packet));
        wait(world, im_transmit_speed, src, dst);
        chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, IO_RECV, id_rx, NULL, 0));
    }


    { // configure - rx
        word_t target = coord_to_u64(src);
        chunk_io(chunk_dst, IO_TARGET, 0, id_rx, &target, 1);
    }

    // basics
    for (size_t it = 0; it < 5; ++it) {
        chunk_io(chunk_src, IO_TRANSMIT, 0, id_tx, packet, array_len(packet));

        wait(world, im_transmit_speed, src, dst);

        chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, IO_RECV, id_rx, packet, array_len(packet)));

        chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, IO_RECV, id_rx, NULL, 0));
    }

    // oversaturate the rx buffer
    for (size_t it = 0; it < 5; ++it) {
        chunk_io(chunk_src, IO_TRANSMIT, 0, id_tx, packet, array_len(packet));
        chunk_io(chunk_src, IO_TRANSMIT, 0, id_tx, packet, array_len(packet));

        wait(world, im_transmit_speed, src, dst);

        chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, IO_RECV, id_rx, packet, array_len(packet)));

        chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, IO_RECV, id_rx, NULL, 0));
    }

    { // reset
        chunk_io(chunk_src, IO_RESET, 0, id_tx, NULL, 0);
        chunk_io(chunk_dst, IO_RESET, 0, id_rx, NULL, 0);

        chunk_io(chunk_src, IO_TRANSMIT, 0, id_tx, packet, array_len(packet));
        wait(world, im_transmit_speed, src, dst);
        chunk_io(chunk_dst, IO_RECEIVE, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, IO_RECV, id_rx, NULL, 0));

    }

    world_free(world);
}
