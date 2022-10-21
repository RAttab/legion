/* storage_test.c
   Rémi Attab (remi.attab@gmail.com), 30 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/storage/storage.h"
#include "items/test/test.h"


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

    const im_id extract_id = make_im_id(item_extract, 1);
    const im_id printer_id = make_im_id(item_printer, 1);

    chunk_create(chunk, item_storage);
    const im_id storage_id = make_im_id(item_storage, 1);
    chunk_create(chunk, item_storage);
    const im_id storage_muscle_id = make_im_id(item_storage, 2);

    chunk_create(chunk, item_test);
    const im_id test_id = make_im_id(item_test, 1);

    const im_id sys_id = 0;
    const vm_word item_elem_a = item_elem_a;
    const vm_word item_muscle = item_muscle;

    // need to make one step for the items to be created.
    world_step(world);

    chunk_io(chunk, io_item, sys_id, storage_id, &item_elem_a, 1);

    for (size_t it = 0; it < 12; ++it) {
        chunk_io(chunk, io_tape, sys_id, extract_id, &item_elem_a, 1);
        step_for(world, 8);
        assert(storage_count(chunk, storage_id, test_id) == 1);
        chunk_io(chunk, io_reset, sys_id, extract_id, NULL, 0);

        chunk_io(chunk, io_tape, sys_id, printer_id, &item_muscle, 1);
        step_for(world, 8);
        assert(storage_count(chunk, storage_id, test_id) == 0);
        chunk_io(chunk, io_reset, sys_id, printer_id, NULL, 0);
    }

    chunk_io(chunk, io_item, sys_id, storage_id, &item_elem_a, 1);
    chunk_io(chunk, io_item, sys_id, storage_muscle_id, &item_muscle, 1);

    for (size_t it = 0; it < 10; ++it) {
        chunk_io(chunk, io_tape, sys_id, extract_id, &item_elem_a, 1);
        chunk_io(chunk, io_tape, sys_id, printer_id, &item_muscle, 1);

        step_for(world, 100);
        assert(storage_count(chunk, storage_muscle_id, test_id) > 1);

        chunk_io(chunk, io_reset, sys_id, extract_id, NULL, 0);
        chunk_io(chunk, io_reset, sys_id, printer_id, NULL, 0);
    }
}
