/* chunk_test.c
   Rémi Attab (remi.attab@gmail.com), 27 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "game/sector.h"


void test_ports_1on1(void)
{
    struct star star = {0};
    struct chunk *chunk = chunk_alloc(&star);

    item_t item = ITEM_ELEM_A;
    id_t src = make_id(ITEM_MINER, 1);
    id_t dst = make_id(ITEM_PRINTER, 1);

    chunk_create(chunk, id_item(src));
    chunk_create(chunk, id_item(dst));
    chunk_create(chunk, ITEM_WORKER);
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        assert(chunk_ports_produce(chunk, src, item));
        assert(!chunk_ports_produce(chunk, src, item));

        assert(chunk_ports_consume(chunk, dst) == ITEM_NIL);
        chunk_ports_request(chunk, dst, item);
        assert(chunk_ports_consume(chunk, dst) == ITEM_NIL);

        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst) == item);
    }

    chunk_free(chunk);
}

void test_ports_2on1(void)
{
    struct star star = {0};
    struct chunk *chunk = chunk_alloc(&star);

    item_t item = ITEM_ELEM_A;
    id_t src0 = make_id(ITEM_MINER, 1);
    id_t src1 = make_id(ITEM_MINER, 2);
    id_t dst = make_id(ITEM_PRINTER, 1);

    chunk_create(chunk, id_item(src0));
    chunk_create(chunk, id_item(src1));
    chunk_create(chunk, id_item(dst));
    chunk_create(chunk, ITEM_WORKER);
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
}

void test_ports_1on2(void)
{
    struct star star = {0};
    struct chunk *chunk = chunk_alloc(&star);

    item_t item = ITEM_ELEM_A;
    id_t src = make_id(ITEM_MINER, 1);
    id_t dst0 = make_id(ITEM_PRINTER, 1);
    id_t dst1 = make_id(ITEM_PRINTER, 2);

    chunk_create(chunk, id_item(src));
    chunk_create(chunk, id_item(dst0));
    chunk_create(chunk, id_item(dst1));
    chunk_create(chunk, ITEM_WORKER);
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        chunk_ports_produce(chunk, src, item);
        chunk_ports_request(chunk, dst1, item);
        chunk_ports_request(chunk, dst0, item);


        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst0) == ITEM_NIL);
        assert(chunk_ports_consume(chunk, dst1) == item);
        chunk_ports_produce(chunk, src, item);

        chunk_step(chunk);
        assert(chunk_ports_consume(chunk, dst0) == item);
        assert(chunk_ports_consume(chunk, dst1) == ITEM_NIL);
    }
}

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    test_ports_1on1();
    test_ports_2on1();
    test_ports_1on2();

    return 0;
}
