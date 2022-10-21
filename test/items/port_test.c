/* port_test.c
   Rémi Attab (remi.attab@gmail.com), 31 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/port/port.h"


void test_port(void)
{
    struct world *world = world_new(0);
    world_populate(world);

    const user_id user = 0;
    const struct sector *sector = world_sector(world, coord_center());

    struct coord src = sector->stars[0].coord;
    struct chunk *src_chunk = world_chunk_alloc(world, src, user);

    struct coord dst = sector->stars[1].coord;
    struct chunk *dst_chunk = world_chunk_alloc(world, dst, user);

    chunk_create(src_chunk, item_port);
    chunk_create(dst_chunk, item_port);
    const im_id port_id = make_im_id(item_port, 1);

    chunk_create(src_chunk, item_storage);
    chunk_create(dst_chunk, item_storage);
    const im_id storage_id = make_im_id(item_storage, 1);

    chunk_create(src_chunk, item_test);
    chunk_create(dst_chunk, item_test);
    const im_id test_id = make_im_id(item_test, 1);

    chunk_create(src_chunk, item_worker);
    chunk_create(dst_chunk, item_worker);

    const im_id sys_id = 0;
    const vm_word item_elem_a = item_elem_a;

    // need to make one step for the items to be created.
    world_step(world);

    enum { load_ticks = 20, elem_count = 2 };

    const vm_word pill_data = cargo_to_word(make_cargo(item_elem_a, elem_count));
    chunk_lanes_arrive(src_chunk, item_pill, dst, &pill_data, 1);
    chunk_lanes_arrive(dst_chunk, item_pill, src, &pill_data, 1);

    chunk_io(src_chunk, io_item, sys_id, storage_id, &item_elem_a, 1);
    chunk_io(dst_chunk, io_item, sys_id, storage_id, &item_elem_a, 1);

    const vm_word dst_data = coord_to_u64(dst);
    const vm_word src_data = coord_to_u64(src);
    const vm_word item_a_data[] = { item_elem_a, elem_count };
    const vm_word item_nil_data = item_nil;


    for (size_t it = 0; it < 10; ++it) {
        chunk_io(src_chunk, io_item, sys_id, port_id, &item_nil_data, 1);
        chunk_io(dst_chunk, io_item, sys_id, port_id, &item_nil_data, 1);
        chunk_io(src_chunk, io_target, sys_id, port_id, &dst_data, 1);
        chunk_io(dst_chunk, io_target, sys_id, port_id, &src_data, 1);
        chunk_io(src_chunk, io_activate, sys_id, port_id, NULL, 0);
        chunk_io(dst_chunk, io_activate, sys_id, port_id, NULL, 0);

        step_for(world, load_ticks);

        assert(storage_count(src_chunk, storage_id, test_id) == elem_count - 1);
        assert(storage_count(dst_chunk, storage_id, test_id) == elem_count - 1);
        chunk_io(src_chunk, io_reset, sys_id, port_id, NULL, 0);
        chunk_io(dst_chunk, io_reset, sys_id, port_id, NULL, 0);

        wait_travel(world, im_port_speed, src, dst);

        chunk_io(src_chunk, io_item, sys_id, port_id, item_a_data, array_len(item_a_data));
        chunk_io(dst_chunk, io_item, sys_id, port_id, item_a_data, array_len(item_a_data));
        chunk_io(src_chunk, io_target, sys_id, port_id, &dst_data, 1);
        chunk_io(dst_chunk, io_target, sys_id, port_id, &src_data, 1);
        chunk_io(src_chunk, io_activate, sys_id, port_id, NULL, 0);
        chunk_io(dst_chunk, io_activate, sys_id, port_id, NULL, 0);

        step_for(world, load_ticks);

        assert(storage_count(src_chunk, storage_id, test_id) == 0);
        assert(storage_count(dst_chunk, storage_id, test_id) == 0);
        chunk_io(src_chunk, io_reset, sys_id, port_id, NULL, 0);
        chunk_io(dst_chunk, io_reset, sys_id, port_id, NULL, 0);

        wait_travel(world, im_port_speed, src, dst);
    }
}
