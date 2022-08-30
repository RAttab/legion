/* storage.c
   Rémi Attab (remi.attab@gmail.com), 30 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/storage/storage.h"
#include "items/test/test.h"


// -----------------------------------------------------------------------------
// test
// -----------------------------------------------------------------------------

void step_for(struct world *world, world_ts_t ticks)
{
    for (world_ts_t ts = 0; ts < ticks; ++ts) world_step(world);
}

size_t storage_count(struct chunk *chunk, id_t storage_id, id_t test_id)
{
    const word_t io_loop = IO_LOOP;
    const struct im_test *test = chunk_get(chunk, test_id);

    chunk_io(chunk, IO_STATE, test_id, storage_id, &io_loop, 1);
    assert(test->io == IO_RETURN);
    assert(test->src == storage_id);
    assert(test->len == 1);

    return test->args[0];
}

void test_storage(void)
{
    struct world *world = world_new(0);
    world_populate(world);

    const uid_t user = 0;
    world_populate_user(world, user);
    struct coord home = world_home(world, user);
    struct chunk *chunk = world_chunk(world, home);

    const id_t extract_id = make_id(ITEM_EXTRACT, 1);
    const id_t printer_id = make_id(ITEM_PRINTER, 1);

    chunk_create(chunk, ITEM_STORAGE);
    const id_t storage_id = make_id(ITEM_STORAGE, 1);
    chunk_create(chunk, ITEM_STORAGE);
    const id_t storage_muscle_id = make_id(ITEM_STORAGE, 2);

    chunk_create(chunk, ITEM_TEST);
    const id_t test_id = make_id(ITEM_TEST, 1);

    const id_t sys_id = 0;
    const word_t item_elem_a = ITEM_ELEM_A;
    const word_t item_muscle = ITEM_MUSCLE;

    // need to make one step for the items to be created.
    world_step(world);

    chunk_io(chunk, IO_ITEM, sys_id, storage_id, &item_elem_a, 1);

    for (size_t it = 0; it < 10; ++it) {
        chunk_io(chunk, IO_TAPE, sys_id, extract_id, &item_elem_a, 1);
        step_for(world, 6);
        assert(storage_count(chunk, storage_id, test_id) == 1);
        chunk_io(chunk, IO_RESET, sys_id, extract_id, NULL, 0);

        chunk_io(chunk, IO_TAPE, sys_id, printer_id, &item_muscle, 1);
        step_for(world, 6);
        assert(storage_count(chunk, storage_id, test_id) == 0);
        chunk_io(chunk, IO_RESET, sys_id, printer_id, NULL, 0);
    }

    chunk_io(chunk, IO_ITEM, sys_id, storage_id, &item_elem_a, 1);
    chunk_io(chunk, IO_ITEM, sys_id, storage_muscle_id, &item_muscle, 1);

    for (size_t it = 0; it < 10; ++it) {
        chunk_io(chunk, IO_TAPE, sys_id, extract_id, &item_elem_a, 1);
        chunk_io(chunk, IO_TAPE, sys_id, printer_id, &item_muscle, 1);

        step_for(world, 100);
        assert(storage_count(chunk, storage_muscle_id, test_id) > 1);

        chunk_io(chunk, IO_RESET, sys_id, extract_id, NULL, 0);
        chunk_io(chunk, IO_RESET, sys_id, printer_id, NULL, 0);
    }
}
