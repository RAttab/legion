/* ring_test.c
   Rémi Attab (remi.attab@gmail.com), 30 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/ring.h"


#define check_same(old, new) assert(*(old) == (new))
#define check_grew(old, _new)                   \
    ({                                          \
        typeof(_new) new = (_new);              \
        assert(*(old) != new);                  \
        *(old) = new;                           \
        new;                                    \
    })

void check_basics(void)
{
    struct ring32 *ring = ring32_reserve(0);
    struct ring32 *old = ring;

    assert(ring32_len(ring) == 0);
    assert(ring32_empty(ring));
    assert(0 == ring32_peek(ring));
    assert(0 == ring32_pop(ring));

    check_same(&old, ring32_push(ring, 1));
    assert(ring32_len(ring) == 1);
    assert(!ring32_empty(ring));

    ring = check_grew(&old, ring32_push(ring, 2));
    ring = check_grew(&old, ring32_push(ring, 3));
    check_same(&old, ring32_push(ring, 4));
    assert(ring32_len(ring) == 4);

    assert(1 == ring32_peek(ring));
    assert(1 == ring32_pop(ring));
    assert(ring32_len(ring) == 3);
    assert(!ring32_empty(ring));

    assert(2 == ring32_pop(ring));
    assert(3 == ring32_pop(ring));
    assert(4 == ring32_pop(ring));
    assert(ring32_len(ring) == 0);
    assert(ring32_empty(ring));

    ring32_free(ring);
}

void check_cap(void)
{
    struct ring32 *ring = ring32_reserve(4);
    struct ring32 *old = ring;

    for (size_t attempt = 0; attempt < 3; ++attempt) {

        for (size_t i = 0; i < ring32_cap(ring); ++i)
            check_same(&old, ring32_push(ring, i));

        for (size_t i = 0; i < ring32_cap(ring); ++i)
            assert(i == ring32_pop(ring));
    }

    ring32_free(ring);
}

void check_growth(void)
{
    struct ring32 *ring = ring32_reserve(1);
    struct ring32 *old = ring;

    check_same(&old, ring32_push(ring, 1));
    assert(1 == ring32_pop(ring));

    check_same(&old, ring32_push(ring, 2));
    ring = check_grew(&old, ring32_push(ring, 3));
    assert(ring->head == 3);
    assert(ring->tail == 1);
    assert(2 == ring32_pop(ring));
    assert(3 == ring32_pop(ring));

    ring32_free(ring);
}

void check_limits(void)
{
    enum { n = 4 };

    struct ring32 *ring = ring32_reserve(n);
    struct ring32 *old = ring;

    for (size_t attempt = 0; attempt < 1; ++attempt) {
        assert(ring->head == ring->tail);
        ring->head = UINT16_MAX - (n/2);
        ring->tail = ring->head;

        for (size_t i = 0; i < n; ++i)
            check_same(&old, ring32_push(ring, i));
        assert(ring32_len(ring) == n);
        assert(!ring32_empty(ring));

        for (size_t i = 0; i < n; ++i)
            assert(i == ring32_pop(ring));
        assert(ring32_len(ring) == 0);
        assert(ring32_empty(ring));
    }

    ring32_free(ring);
}

struct ring32 *ring32_clone(const struct ring32 *old)
{
    struct ring32 *new = ring32_reserve(old->cap);
    memcpy(new, old, sizeof(*old) + old->cap * sizeof(old->vals[0]));
    return new;
}

#define assert_ring_eq(lhs, rhs)                                \
    do {                                                        \
        assert(lhs->head == rhs->head);                         \
        assert(lhs->tail == rhs->tail);                         \
                                                                \
        for (size_t i = 0; i < ring32_len(lhs); ++i) {          \
            assert(lhs->vals[(lhs->tail + i) % lhs->cap] ==     \
                    rhs->vals[(rhs->tail + i) % rhs->cap]);     \
        }                                                       \
    } while (false)

void check_save(size_t init, size_t churn)
{
    struct save *save = save_mem_new();

    ring_it val = 1;
    struct ring32 *base = ring32_reserve(init);
    while (val < init) base = ring32_push(base, val++);

    struct ring32 *ring = NULL;

    for (size_t it = 0; it < (ring_cap * 2) / churn; ++it) {
        save_mem_reset(save);
        ring32_save(base, save);
        save_mem_reset(save);

        ring32_free(ring);
        assert((ring = ring32_load(save)));
        assert_ring_eq(base, ring);

        for (size_t i = 0; i < churn; ++i) base = ring32_push(base, val++);
        for (size_t i = 0; i < churn; ++i) ring32_pop(base);
    }

    save_mem_free(save);
    ring32_free(base);
    ring32_free(ring);
}

void check_save_delta(size_t init, size_t churn)
{
    struct save *save = save_mem_new();

    ring_it val = 1;
    struct ring32 *base = ring32_reserve(init);
    while (val < init) base = ring32_push(base, val++);

    struct ring_ack ack = {0};
    struct ring32 *ring = NULL;

    for (size_t it = 0; it < (ring_cap * 2) / churn; ++it) {
        save_mem_reset(save);
        ring32_save_delta(base, save, &ack);
        save_mem_reset(save);

        assert(ring32_load_delta(&ring, save, &ack));
        assert_ring_eq(base, ring);

        for (size_t i = 0; i < churn; ++i) base = ring32_push(base, val++);
        for (size_t i = 0; i < churn; ++i) ring32_pop(base);
    }

    save_mem_free(save);
    ring32_free(base);
    ring32_free(ring);
}

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    check_basics();
    check_cap();
    check_growth();
    check_limits();
    check_save(100, 50);
    check_save(100, 150);
    check_save_delta(100, 50);
    check_save_delta(100, 150);

    return 0;
};
