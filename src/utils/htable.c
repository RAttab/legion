/* htable.c
   Rémi Attab (remi.attab@gmail.com), 10 Mar 2016
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/htable.h"
#include "utils/hash.h"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

enum { htable_window = 8 };


// -----------------------------------------------------------------------------
// htable
// -----------------------------------------------------------------------------

void htable_clear(struct htable *ht)
{
    if (!ht->cap) return;

    memset(ht->table, 0, ht->cap * sizeof(*ht->table));
    ht->len = 0;
}

void htable_reset(struct htable *ht)
{
    free(ht->table);
    *ht = (struct htable) {0};
}

static bool table_put(
        struct htable_bucket *table, size_t cap,
        uint64_t key, uint64_t value)
{
    assert(key);
    uint64_t hash = hash_u64(key);

    for (size_t i = 0; i < htable_window; ++i) {
        struct htable_bucket *bucket = &table[(hash + i) % cap];
        if (bucket->key) continue;

        bucket->key = key;
        bucket->value = value;
        return true;
    }

    return false;
}

static void htable_resize(struct htable *ht, size_t cap)
{
    if (cap <= ht->cap) return;

    size_t new_cap = ht->cap ? ht->cap : htable_window;
    while (new_cap < cap) new_cap *= 2;

    struct htable_bucket *new_table = calloc(new_cap, sizeof(*new_table));
    for (size_t i = 0; i < ht->cap; ++i) {
        struct htable_bucket *bucket = &ht->table[i];
        if (!bucket->key) continue;

        if (!table_put(new_table, new_cap, bucket->key, bucket->value)) {
            free(new_table);
            htable_resize(ht, new_cap * 2);
            return;
        }
    }

    free(ht->table);
    ht->cap = new_cap;
    ht->table = new_table;
}

void htable_reserve(struct htable *ht, size_t items)
{
    htable_resize(ht, items * 2);
}

struct htable htable_clone(const struct htable *src)
{
    struct htable dst = { .len = src->len, .cap = src->cap };
    dst.table = calloc(dst.cap, sizeof(*dst.table));
    memcpy(dst.table, src->table, dst.cap * sizeof(*dst.table));
    return dst;
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

struct htable_ret htable_get(const struct htable *ht, uint64_t key)
{
    assert(key);

    if (!ht->cap) return (struct htable_ret) { .ok = false };
    assert(ht->cap >= htable_window);

    uint64_t hash = hash_u64(key);
    for (size_t i = 0; i < htable_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (!bucket->key) continue;
        if (bucket->key != key) continue;

        return (struct htable_ret) { .ok = true, .value = bucket->value };
    }

    return (struct htable_ret) { .ok = false };
}


static bool htable_try_put(
        struct htable *ht, uint64_t key, uint64_t value, struct htable_ret *ret)
{
    assert(key);

    uint64_t hash = hash_u64(key);
    htable_resize(ht, htable_window);

    for (size_t i = 0; i < htable_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (bucket->key) {
            if (bucket->key != key) continue;
            *ret = (struct htable_ret) { .ok = false, .value = bucket->value };
            return true;
        }

        ht->len++;
        bucket->key = key;
        bucket->value = value;
        *ret = (struct htable_ret) { .ok = true };
        return true;
    }

    return false;
}

struct htable_ret htable_put(struct htable *ht, uint64_t key, uint64_t value)
{
    assert(key);

    struct htable_ret ret = {0};
    if (htable_try_put(ht, key, value, &ret)) return ret;

    htable_resize(ht, ht->cap * 2);
    return htable_put(ht, key, value);
}

struct htable_ret htable_xchg(struct htable *ht, uint64_t key, uint64_t value)
{
    assert(key);

    uint64_t hash = hash_u64(key);
    htable_resize(ht, htable_window);

    for (size_t i = 0; i < htable_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];
        if (bucket->key != key) continue;

        uint64_t old_value = bucket->value;
        bucket->value = value;
        return (struct htable_ret) { .ok = true, .value = old_value };
    }

    return (struct htable_ret) { .ok = false };
}

struct htable_ret htable_del(struct htable *ht, uint64_t key)
{
    assert(key);

    uint64_t hash = hash_u64(key);
    htable_resize(ht, htable_window);

    for (size_t i = 0; i < htable_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];
        if (bucket->key != key) continue;

        ht->len--;
        bucket->key = 0;
        bucket->value = 0;
        return (struct htable_ret) { .ok = true, .value = bucket->value };
    }

    return (struct htable_ret) { .ok = false };
}

bool htable_eq(const struct htable *lhs, const struct htable *rhs)
{
    if (lhs->len != rhs->len) return false;

    for (const struct htable_bucket *it = htable_next(lhs, NULL);
         it; it = htable_next(lhs, it))
    {
        struct htable_ret ret = htable_get(rhs, it->key);
        if (!ret.ok || ret.value != it->value) return false;
    }

    return true;
}


const struct htable_bucket *htable_next(
        const struct htable *ht, const struct htable_bucket *bucket)
{
    if (!ht->table) return NULL;

    size_t i = 0;
    if (bucket) i = (bucket - ht->table) + 1;

    for (; i < ht->cap; ++i) {
        bucket = &ht->table[i];
        if (bucket->key) return bucket;
    }

    return NULL;
}
