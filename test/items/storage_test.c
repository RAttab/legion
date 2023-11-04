/* storage_test.c
   Rémi Attab (remi.attab@gmail.com), 30 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// test
// -----------------------------------------------------------------------------

void test_storage(void)
{
    struct world *world = world_new(0);
    world_populate(world);

    const user_id user = 0;
    world_populate_user(world, user);
    struct coord home = world_home(world, user);
    struct chunk *chunk = world_chunk(world, home);

    chunk_create(chunk, item_solar);

    const im_id extract_elem_b_id = make_im_id(item_extract, 1);
    const im_id extract_elem_d_id = make_im_id(item_extract, 2);

    chunk_create(chunk, item_storage);
    const im_id storage_id = make_im_id(item_storage, 1);
    chunk_create(chunk, item_storage);
    const im_id storage_d_id = make_im_id(item_storage, 2);

    chunk_create(chunk, item_test);
    const im_id test_id = make_im_id(item_test, 1);

    const im_id sys_id = 0;
    const vm_word im_elem_b = item_elem_b;
    const vm_word im_elem_d = item_elem_d;

    // need to make one step for the items to be created.
    world_step(world);

    chunk_io(chunk, io_item, sys_id, storage_id, &im_elem_b, 1);

    for (size_t it = 0; it < 10; ++it) {
        chunk_io(chunk, io_tape, sys_id, extract_elem_b_id, &im_elem_b, 1);
        step_for(world, 10);
        assert(storage_count(chunk, storage_id, test_id) == 1);
        chunk_io(chunk, io_reset, sys_id, extract_elem_b_id, NULL, 0);

        chunk_io(chunk, io_tape, sys_id, extract_elem_d_id, &im_elem_d, 1);
        step_for(world, 10);
        assert(storage_count(chunk, storage_id, test_id) == 0);
        chunk_io(chunk, io_reset, sys_id, extract_elem_d_id, NULL, 0);
    }

    chunk_io(chunk, io_item, sys_id, storage_id, &im_elem_b, 1);
    chunk_io(chunk, io_item, sys_id, storage_d_id, &im_elem_d, 1);

    for (size_t it = 0; it < 10; ++it) {
        chunk_io(chunk, io_tape, sys_id, extract_elem_b_id, &im_elem_b, 1);
        chunk_io(chunk, io_tape, sys_id, extract_elem_d_id, &im_elem_d, 1);

        step_for(world, 100);
        assert(storage_count(chunk, storage_d_id, test_id) > 1);

        chunk_io(chunk, io_reset, sys_id, extract_elem_b_id, NULL, 0);
        chunk_io(chunk, io_reset, sys_id, extract_elem_d_id, NULL, 0);
    }

    world_free(world);
}
