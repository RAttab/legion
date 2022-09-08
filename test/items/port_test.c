/* port_test.c
   Rémi Attab (remi.attab@gmail.com), 31 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/port/port.h"


void test_port(void)
{
    struct world *world = world_new(0);
    world_populate(world);

    const user user = 0;
    const struct sector *sector = world_sector(world, coord_center());

    struct coord src = sector->stars[0].coord;
    struct chunk *src_chunk = world_chunk_alloc(world, src, user);

    struct coord dst = sector->stars[1].coord;
    struct chunk *dst_chunk = world_chunk_alloc(world, dst, user);

    chunk_create(src_chunk, ITEM_PORT);
    chunk_create(dst_chunk, ITEM_PORT);
    const id_t port_id = make_id(ITEM_PORT, 1);

    chunk_create(src_chunk, ITEM_STORAGE);
    chunk_create(dst_chunk, ITEM_STORAGE);
    const id_t storage_id = make_id(ITEM_STORAGE, 1);

    chunk_create(src_chunk, ITEM_TEST);
    chunk_create(dst_chunk, ITEM_TEST);
    const id_t test_id = make_id(ITEM_TEST, 1);

    chunk_create(src_chunk, ITEM_WORKER);
    chunk_create(dst_chunk, ITEM_WORKER);

    const id_t sys_id = 0;
    const word item_elem_a = ITEM_ELEM_A;

    // need to make one step for the items to be created.
    world_step(world);

    const word pill_data = im_port_pack(ITEM_ELEM_A, 2);
    chunk_lanes_arrive(src_chunk, ITEM_PILL, dst, &pill_data, 1);
    chunk_lanes_arrive(dst_chunk, ITEM_PILL, src, &pill_data, 1);

    chunk_io(src_chunk, IO_ITEM, sys_id, storage_id, &item_elem_a, 1);
    chunk_io(dst_chunk, IO_ITEM, sys_id, storage_id, &item_elem_a, 1);

    enum { load_ticks = 6, elem_count = 2 };
    const word dst_data = coord_to_u64(dst);
    const word src_data = coord_to_u64(src);
    const word item_a_data[] = { item_elem_a, elem_count };
    const word item_nil_data = ITEM_NIL;


    for (size_t it = 0; it < 10; ++it) {
        chunk_io(src_chunk, IO_ITEM, sys_id, port_id, &item_nil_data, 1);
        chunk_io(dst_chunk, IO_ITEM, sys_id, port_id, &item_nil_data, 1);
        chunk_io(src_chunk, IO_TARGET, sys_id, port_id, &dst_data, 1);
        chunk_io(dst_chunk, IO_TARGET, sys_id, port_id, &src_data, 1);

        step_for(world, load_ticks);

        assert(storage_count(src_chunk, storage_id, test_id) == elem_count - 1);
        assert(storage_count(dst_chunk, storage_id, test_id) == elem_count - 1);
        chunk_io(src_chunk, IO_RESET, sys_id, port_id, NULL, 0);
        chunk_io(dst_chunk, IO_RESET, sys_id, port_id, NULL, 0);

        wait_travel(world, im_port_speed, src, dst);

        chunk_io(src_chunk, IO_ITEM, sys_id, port_id, item_a_data, array_len(item_a_data));
        chunk_io(dst_chunk, IO_ITEM, sys_id, port_id, item_a_data, array_len(item_a_data));
        chunk_io(src_chunk, IO_TARGET, sys_id, port_id, &dst_data, 1);
        chunk_io(dst_chunk, IO_TARGET, sys_id, port_id, &src_data, 1);

        step_for(world, load_ticks);

        assert(storage_count(src_chunk, storage_id, test_id) == 0);
        assert(storage_count(dst_chunk, storage_id, test_id) == 0);
        chunk_io(src_chunk, IO_RESET, sys_id, port_id, NULL, 0);
        chunk_io(dst_chunk, IO_RESET, sys_id, port_id, NULL, 0);

        wait_travel(world, im_port_speed, src, dst);
    }
}
