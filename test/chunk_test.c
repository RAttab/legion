/* chunk_test.c
   Rémi Attab (remi.attab@gmail.com), 27 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/game.h"
#include "items/config.h"


void test_ports_1on1(void)
{
    struct star star = {0};
    struct world *world = world_new(0);
    struct chunk *chunk = chunk_alloc(world, &star, user_admin, 0);

    enum item item = item_elem_a;
    im_id src = make_im_id(item_extract, 1);
    im_id dst = make_im_id(item_printer, 1);

    chunk_create(chunk, im_id_item(src));
    chunk_create(chunk, im_id_item(dst));
    chunk_create(chunk, item_worker);
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        assert(chunk_ports_produce(chunk, src, item));
        assert(!chunk_ports_produce(chunk, src, item));

        assert(chunk_ports_consume(chunk, dst) == item_nil);
        chunk_ports_request(chunk, dst, item);
        assert(chunk_ports_consume(chunk, dst) == item_nil);

        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item);
    }

    chunk_free(chunk);
    world_free(world);
}

void test_ports_2on1(void)
{
    struct star star = {0};
    struct world *world = world_new(0);
    struct chunk *chunk = chunk_alloc(world, &star, user_admin, 0);

    enum item item = item_elem_a;
    im_id src0 = make_im_id(item_extract, 1);
    im_id src1 = make_im_id(item_extract, 2);
    im_id dst = make_im_id(item_printer, 1);

    chunk_create(chunk, im_id_item(src0));
    chunk_create(chunk, im_id_item(src1));
    chunk_create(chunk, im_id_item(dst));
    chunk_create(chunk, item_worker);
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        assert(chunk_ports_produce(chunk, src1, item));
        assert(chunk_ports_produce(chunk, src0, item));
        chunk_ports_request(chunk, dst, item);

        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item);
        chunk_ports_request(chunk, dst, item);

        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item);
    }

    chunk_free(chunk);
    world_free(world);
}

void test_ports_1on2(void)
{
    struct star star = {0};
    struct world *world = world_new(0);
    struct chunk *chunk = chunk_alloc(world, &star, user_admin, 0);

    enum item item = item_elem_a;
    im_id src = make_im_id(item_extract, 1);
    im_id dst0 = make_im_id(item_printer, 1);
    im_id dst1 = make_im_id(item_printer, 2);

    chunk_create(chunk, im_id_item(src));
    chunk_create(chunk, im_id_item(dst0));
    chunk_create(chunk, im_id_item(dst1));
    chunk_create(chunk, item_worker);
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        chunk_ports_produce(chunk, src, item);
        chunk_ports_request(chunk, dst1, item);
        chunk_ports_request(chunk, dst0, item);


        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst0) == item_nil);
        assert(chunk_ports_consume(chunk, dst1) == item);
        chunk_ports_produce(chunk, src, item);

        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst0) == item);
        assert(chunk_ports_consume(chunk, dst1) == item_nil);
    }

    chunk_free(chunk);
    world_free(world);
}

void test_ports_reset(void)
{
    struct star star = {0};
    struct world *world = world_new(0);
    struct chunk *chunk = chunk_alloc(world, &star, user_admin, 0);

    enum item item = item_elem_a;
    im_id src = make_im_id(item_extract, 1);
    im_id dst = make_im_id(item_printer, 1);

    chunk_create(chunk, im_id_item(src));
    chunk_create(chunk, im_id_item(dst));
    chunk_create(chunk, item_worker);
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        chunk_ports_produce(chunk, src, item);
        chunk_ports_request(chunk, dst, item);
        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item);
        assert(chunk_workers(chunk).queue == 1);

        chunk_ports_produce(chunk, src, item);
        chunk_ports_reset(chunk, src);
        chunk_ports_request(chunk, dst, item);
        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item_nil);
        assert(chunk_workers(chunk).queue == 1);
        assert(chunk_workers(chunk).clean == 1);
        assert(chunk_workers(chunk).fail == 1);

        chunk_ports_produce(chunk, src, item);
        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item);
        assert(chunk_workers(chunk).queue == 1);

        chunk_ports_produce(chunk, src, item);
        chunk_ports_request(chunk, dst, item);
        chunk_ports_reset(chunk, dst);
        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item_nil);
        assert(chunk_workers(chunk).queue == 1);
        assert(chunk_workers(chunk).clean == 1);
        assert(chunk_workers(chunk).fail == 0);

        chunk_ports_request(chunk, dst, item);
        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item);
        assert(chunk_workers(chunk).queue == 1);

        chunk_ports_produce(chunk, src, item);
        chunk_ports_request(chunk, dst, item);
        chunk_ports_reset(chunk, src);
        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item_nil);
        assert(chunk_workers(chunk).queue == 1);
        assert(chunk_workers(chunk).clean == 1);
        assert(chunk_workers(chunk).fail == 1);

        chunk_ports_reset(chunk, dst);
        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item_nil);
        assert(chunk_workers(chunk).queue == 1);
        assert(chunk_workers(chunk).clean == 1);
        assert(chunk_workers(chunk).fail == 0);
    }

    chunk_free(chunk);
    world_free(world);
}

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    sys_populate_tests();

    test_ports_1on1();
    test_ports_2on1();
    test_ports_1on2();
    test_ports_reset();

    return 0;
}
