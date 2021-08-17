/* hset.c
   Rémi Attab (remi.attab@gmail.com), 17 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/hset.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

enum { hset_window = 8 };


// -----------------------------------------------------------------------------
// hset
// -----------------------------------------------------------------------------

void hset_free(struct hset *hs)
{
    free(hs);
}

static bool set_put(struct hset *set, size_t cap, uint64_t key)
{
    assert(key);
    uint64_t hash = hash_u64(key);

    for (size_t i = 0; i < hset_window; ++i) {
        uint64_t *bucket = &set->set[(hash + i) % cap];
        if (*bucket) continue;

        *bucket = key;
        return true;
    }

    return false;
}

static struct hset *hset_resize(struct hset *hs, size_t cap)
{
    if (hs && cap <= hs->cap) return hs;

    size_t new_cap = hset_window;
    if (hs && hs->cap > hset_window) new_cap = hs->cap;
    while (new_cap < cap) new_cap *= 2;

    struct hset *new = calloc(1, sizeof(*new) + new_cap * sizeof(new->set[0]));
    new->cap = new_cap;

    if (hs) {
        new->len = hs->len;

        for (uint64_t *it = hs->set; it < hs->set + hs->cap; it++) {
            if (!*it) continue;
            if (!set_put(new, new_cap, *it)) {
                free(new);
                return hset_resize(hs, new_cap * 2);
            }

            free(hs);
        }
    }
    return new;
}

struct hset *hset_reserve(size_t len)
{
    return hset_resize(NULL, len * 2);
}

struct hset *hset_clone(const struct hset *src)
{
    if (!src) return NULL;
    struct hset *dst = hset_reserve(src->len);
    memcpy(dst, src, sizeof(*dst) + src->cap * sizeof(dst->set[0]));
    return dst;
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------


bool hset_test(const struct hset *hs, uint64_t key)
{
    assert(key);
    if (!hs) return false;

    const uint64_t hash = hash_u64(key);
    for (size_t i = 0; i < hset_window; ++i) {
        const uint64_t *bucket = &hs->set[(hash + i) % hs->cap];
        if (*bucket == key) return true;
    }

    return false;
}


struct hset *hset_put(struct hset *hs, uint64_t key)
{
    assert(key);
    if (!hs) hs = hset_resize(hs, hset_window);

    const uint64_t hash = hash_u64(key);
    for (size_t i = 0; i < hset_window; ++i) {
        uint64_t *bucket = &hs->set[(hash + i) % hs->cap];
        if (*bucket == key) return hs;
        if (*bucket) continue;

        *bucket = key;
        hs->len++;
        return hs;
    }

    return hset_put(hset_resize(hs, hs->cap * 2), key);
}


bool hset_del(struct hset *hs, uint64_t key)
{
    assert(key);
    if (!hs) return false;

    const uint64_t hash = hash_u64(key);
    for (size_t i = 0; i < hset_window; ++i) {
        uint64_t *bucket = &hs->set[(hash + i) % hs->cap];
        if (*bucket != key) continue;

        hs->len--;
        *bucket = 0;
        return true;
    }

    return false;
}


const uint64_t *hset_next(const struct hset *hs, const uint64_t *it)
{
    if (!hs->set) return NULL;

    size_t i = 0;
    if (it) i = (it - hs->set) + 1;

    for (; i < hs->cap; ++i) {
        it = &hs->set[i];
        if (*it) return it;
    }

    return NULL;
}


// -----------------------------------------------------------------------------
// math
// -----------------------------------------------------------------------------

bool hset_eq(const struct hset *lhs, const struct hset *rhs)
{
    if (!lhs || !rhs) return !lhs && !rhs;
    if (lhs->len != rhs->len) return false;

    for (hset_it_t it = hset_next(lhs, NULL); it; it = hset_next(lhs, it))
        if (!hset_test(rhs, *it)) return false;

    return true;
}

struct hset *hset_ret(const struct hset *lhs, const struct hset *rhs)
{
    struct hset *ret = NULL;
    for (hset_it_t it = hset_next(lhs, NULL); it; it = hset_next(lhs, it)) {
        if (!hset_test(rhs, *it)) ret = hset_put(ret, *it);
    }
    return ret;
}

struct hset *hset_union(const struct hset *lhs, const struct hset *rhs)
{
    struct hset *ret = hset_clone(lhs);
    for (hset_it_t it = hset_next(rhs, NULL); it; it = hset_next(rhs, it))
        ret = hset_put(ret, *it);
    return ret;

}

struct hset *hset_intersect(const struct hset *lhs, const struct hset *rhs)
{
    struct hset *ret = NULL;
    for (hset_it_t it = hset_next(lhs, NULL); it; it = hset_next(lhs, it)) {
        if (hset_test(rhs, *it)) ret = hset_put(ret, *it);
    }
    return ret;
}


// -----------------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------------

size_t hset_str(const struct hset *hs, char *str, size_t len)
{
    const char *base = str;
    const char *end = str + len;

    str += snprintf(str, end - str, "{ ");
    for (hset_it_t it = hset_next(hs, NULL); it; it = hset_next(hs, it))
        str += snprintf(str, end - str, "%lx ", *it);
    str += snprintf(str, end - str, "}");

    return str - base;
}
