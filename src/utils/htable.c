/* htable.c
   Rémi Attab (remi.attab@gmail.com), 10 Mar 2016
   FreeBSD-style copyright and disclaimer apply
*/

#include "htable.h"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

enum { htable_window = 8 };


// -----------------------------------------------------------------------------
// hash
// -----------------------------------------------------------------------------

// xorshift - need to compare against fnv-1a
static inline uint64_t hash_key(uint64_t key)
{
    // We xor the seed with a randomly chosen number to avoid ending up with a 0
    // state which would be bad.
    key ^= UINT64_C(0xedef335f00e170b3);
    key ^= key >> 12;
    key ^= key << 25;
    key ^= key >> 27;
    key *= UINT64_C(2685821657736338717);
    assert(key); // can't be 0
    return key;
}


// -----------------------------------------------------------------------------
// htable
// -----------------------------------------------------------------------------

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
    uint64_t hash = hash_key(key);

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

    size_t new_cap = ht->cap ? ht->cap : 1;
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
    htable_resize(ht, items * 4);
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

struct htable_ret htable_get(struct htable *ht, uint64_t key)
{
    assert(key);

    uint64_t hash = hash_key(key);
    htable_resize(ht, htable_window);

    for (size_t i = 0; i < htable_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (!bucket->key) continue;
        if (bucket->key != key) continue;

        return (struct htable_ret) { .ok = true, .value = bucket->value };
    }

    return (struct htable_ret) { .ok = false };
}


struct htable_ret htable_try_put(struct htable *ht, uint64_t key, uint64_t value)
{
    assert(key);

    uint64_t hash = hash_key(key);
    htable_resize(ht, htable_window);

    for (size_t i = 0; i < htable_window; ++i) {
        struct htable_bucket *bucket = &ht->table[(hash + i) % ht->cap];

        if (bucket->key) {
            if (bucket->key != key) continue;
            return (struct htable_ret) { .ok = false, .value = bucket->value };
        }

        ht->len++;
        bucket->key = key;
        bucket->value = value;
        return (struct htable_ret) { .ok = true };
    }

    return (struct htable_ret) { .ok = false };
}

struct htable_ret htable_put(struct htable *ht, uint64_t key, uint64_t value)
{
    struct htable_ret ret = htable_try_put(ht, key, value);
    if (ret.ok || ret.value) return ret;

    htable_resize(ht, ht->cap * 2);
    return htable_put(ht, key, value);
}

struct htable_ret htable_xchg(struct htable *ht, uint64_t key, uint64_t value)
{
    assert(key);

    uint64_t hash = hash_key(key);
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

    uint64_t hash = hash_key(key);
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


struct htable_bucket * htable_next(
        struct htable *ht, struct htable_bucket *bucket)
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
