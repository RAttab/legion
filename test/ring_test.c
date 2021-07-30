/* ring_test.c
   Rémi Attab (remi.attab@gmail.com), 30 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/ring.h"


#define check_same(old, new) assert(*(old) == (new))
#define check_grew(old, _new)                   \
    ({                                          \
        typeof(_new) new = _new;                \
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

    ring = check_grew(&old, ring32_push(ring, 1));
    assert(ring32_len(ring) == 1);
    assert(!ring32_empty(ring));

    ring = check_grew(&old, ring32_push(ring, 2));
    ring = check_grew(&old, ring32_push(ring, 3));
    check_same(&old, ring32_push(ring, 4));
    assert(ring32_len(ring) == 4);

    ring32_replace(ring, 2, 5);

    assert(1 == ring32_peek(ring));
    assert(1 == ring32_pop(ring));
    assert(ring32_len(ring) == 3);
    assert(!ring32_empty(ring));

    assert(5 == ring32_pop(ring));
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

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    check_basics();
    check_cap();
    check_limits();

    return 0;
};
