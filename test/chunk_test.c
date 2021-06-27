/* chunk_test.c
   Rémi Attab (remi.attab@gmail.com), 27 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "game/galaxy.h"


void check_pair(struct chunk *chunk, item_t exp_item, id_t exp_src, id_t exp_dst)
{
    item_t item = 0;
    id_t src = 0, dst = 0;

    bool ret = chunk_ports_pair(chunk, &item, &src, &dst);

    assert(exp_item ? ret : !ret);
    assert(exp_item == item);
    assert(exp_src == src);
    assert(exp_dst == dst);
}

void test_ports_1on1(void)
{
    struct star star = {0};
    struct chunk *chunk = chunk_alloc(&star);

    item_t item = ITEM_ELEM_A;
    id_t src = make_id(ITEM_MINER, 1);
    id_t dst = make_id(ITEM_PRINTER, 1);

    chunk_create(chunk, id_item(src));
    chunk_create(chunk, id_item(dst));
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        check_pair(chunk, 0, 0, 0);

        assert(chunk_ports_produce(chunk, src, item));
        assert(!chunk_ports_produce(chunk, src, item));

        check_pair(chunk, 0, 0, 0);

        assert(chunk_ports_consume(chunk, dst) == ITEM_NIL);
        chunk_ports_request(chunk, dst, item);
        assert(chunk_ports_consume(chunk, dst) == ITEM_NIL);

        check_pair(chunk, item, src, dst);

        assert(!chunk_ports_produce(chunk, src, item));
        assert(chunk_ports_take(chunk, src) == item);

        check_pair(chunk, 0, 0, 0);

        assert(chunk_ports_consume(chunk, dst) == ITEM_NIL);
        chunk_ports_give(chunk, dst, item);
        assert(chunk_ports_consume(chunk, dst) == item);
        assert(chunk_ports_consume(chunk, dst) == ITEM_NIL);

        check_pair(chunk, 0, 0, 0);
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
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        assert(chunk_ports_produce(chunk, src1, item));
        assert(chunk_ports_produce(chunk, src0, item));

        chunk_ports_request(chunk, dst, item);
        check_pair(chunk, item, src1, dst);
        assert(chunk_ports_take(chunk, src1) == item);
        chunk_ports_give(chunk, dst, item);
        assert(chunk_ports_consume(chunk, dst) == item);

        chunk_ports_request(chunk, dst, item);
        check_pair(chunk, item, src0, dst);
        assert(chunk_ports_take(chunk, src0) == item);
        chunk_ports_give(chunk, dst, item);
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
    chunk_step(chunk);

    for (size_t i = 0; i < 3; ++i) {
        chunk_ports_request(chunk, dst1, item);
        chunk_ports_request(chunk, dst0, item);

        assert(chunk_ports_produce(chunk, src, item));
        check_pair(chunk, item, src, dst1);
        assert(chunk_ports_take(chunk, src) == item);
        chunk_ports_give(chunk, dst1, item);
        assert(chunk_ports_consume(chunk, dst1) == item);

        chunk_ports_produce(chunk, src, item);
        check_pair(chunk, item, src, dst0);
        assert(chunk_ports_take(chunk, src) == item);
        chunk_ports_give(chunk, dst0, item);
        assert(chunk_ports_consume(chunk, dst0) == item);
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
