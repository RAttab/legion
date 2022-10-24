/* txrx_test.c
   Rémi Attab (remi.attab@gmail.com), 28 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/transmit/transmit.h"
#include "db/specs.h"


// -----------------------------------------------------------------------------
// test
// -----------------------------------------------------------------------------

void test_txrx(void)
{
    struct world *world = world_new(0);
    world_populate(world);

    const struct sector *sector = world_sector(world, coord_center());
    const struct coord src = sector->stars[0].coord;
    const struct coord dst = sector->stars[1].coord;
    struct chunk *chunk_src = world_chunk_alloc(world, src, user_admin);
    struct chunk *chunk_dst = world_chunk_alloc(world, dst, user_admin);

    chunk_create(chunk_src, item_fusion);
    chunk_create(chunk_src, item_fusion);
    chunk_create(chunk_src, item_transmit);
    chunk_create(chunk_dst, item_receive);
    chunk_create(chunk_dst, item_test);
    world_step(world);

    im_id id_tx = make_im_id(item_transmit, 1);
    im_id id_rx = make_im_id(item_receive, 1);
    im_id id_test = make_im_id(item_test, 1);

    const struct im_test *test = chunk_get(chunk_dst, id_test);
    const vm_word packet[im_packet_max] = {
        0x1111111111111111,
        0x2222222222222222,
        0x3333333333333333
    };

    // nil rx
    chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
    assert(im_test_check(test, io_recv, id_rx, NULL, 0));

    { // unconfigured
        chunk_io(chunk_src, io_transmit, 0, id_tx, packet, array_len(packet));
        wait_travel(world, im_transmit_launch_speed, src, dst);
        chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, io_recv, id_rx, NULL, 0));
    }

    { // configure - tx
        vm_word target = coord_to_u64(dst);
        chunk_io(chunk_src, io_target, 0, id_tx, &target, 1);

        chunk_io(chunk_src, io_transmit, 0, id_tx, packet, array_len(packet));
        wait_travel(world, im_transmit_launch_speed, src, dst);
        chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, io_recv, id_rx, NULL, 0));
    }


    { // configure - rx
        vm_word target = coord_to_u64(src);
        chunk_io(chunk_dst, io_target, 0, id_rx, &target, 1);
    }

    // basics
    for (size_t it = 0; it < 5; ++it) {
        chunk_io(chunk_src, io_transmit, 0, id_tx, packet, array_len(packet));

        wait_travel(world, im_transmit_launch_speed, src, dst);

        chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, io_recv, id_rx, packet, array_len(packet)));

        chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, io_recv, id_rx, NULL, 0));
    }

    // oversaturate the rx buffer
    for (size_t it = 0; it < 5; ++it) {
        chunk_io(chunk_src, io_transmit, 0, id_tx, packet, array_len(packet));
        world_step(world);
        chunk_io(chunk_src, io_transmit, 0, id_tx, packet, array_len(packet));

        wait_travel(world, im_transmit_launch_speed, src, dst);

        chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, io_recv, id_rx, packet, array_len(packet)));

        chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, io_recv, id_rx, NULL, 0));
    }

    { // reset
        chunk_io(chunk_src, io_reset, 0, id_tx, NULL, 0);
        chunk_io(chunk_dst, io_reset, 0, id_rx, NULL, 0);

        chunk_io(chunk_src, io_transmit, 0, id_tx, packet, array_len(packet));
        wait_travel(world, im_transmit_launch_speed, src, dst);
        chunk_io(chunk_dst, io_receive, id_test, id_rx, NULL, 0);
        assert(im_test_check(test, io_recv, id_rx, NULL, 0));

    }

    world_free(world);
}
