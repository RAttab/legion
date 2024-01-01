/* save.c
   Rémi Attab (remi.attab@gmail.com), 01 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/save.h"
#include "utils/vec.h"
#include "utils/err.h"

// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

static const uint64_t save_magic_top = 0xFF4E4F4947454CFF;
static const uint64_t save_magic_seal = 0xFF4C4547494F4EFF;
static const size_t save_chunks = 10 * page_len;

typedef void (*save_grow_fn) (struct save *, size_t len);

struct save_prof
{
    size_t depth;
    struct { enum save_magic magic; size_t it; } stack[16];
    size_t bytes[save_magic_len];
};

struct save
{
    void *base, *end, *it;
    save_grow_fn grow;
    struct save_prof *prof;
};

static void save_free(struct save *save)
{
    free(save->prof);
}

bool save_eof(struct save *save)
{
    return save_len(save) == save_cap(save);
}

size_t save_cap(struct save *save)
{
    return save->end - save->base;
}

size_t save_len(struct save *save)
{
    return save->it - save->base;
}

void *save_bytes(struct save *save)
{
    return save->base;
}


// -----------------------------------------------------------------------------
// implementations
// -----------------------------------------------------------------------------

#include "utils/save_mem.c"
#include "utils/save_ring.c"
#include "utils/save_file.c"


// -----------------------------------------------------------------------------
// read/write
// -----------------------------------------------------------------------------

size_t save_write(struct save *save, const void *src, size_t len)
{
    if (unlikely(save->it + len > save->end)) {
        if (save->grow) save->grow(save, len);
        else len = save->end - save->it;
    }

    memcpy(save->it, src, len);
    save->it += len;
    return len;
}

size_t save_read(struct save *save, void *dst, size_t len)
{
    if (unlikely(save->it + len > save->end))
        len = save->end - save->it;

    assert(save_len(save) + len <= save_cap(save));
    memcpy(dst, save->it, len);
    save->it += len;
    return len;
}

size_t save_read_skip(struct save *save, size_t len)
{
    if (unlikely(save->it + len > save->end))
        len = save->end - save->it;

    assert(save_len(save) + len <= save_cap(save));
    save->it += len;
    return len;
}

size_t save_copy(struct save *dst, struct save *src, size_t len)
{
    size_t bytes = save_write(dst, src->it, len);
    save_read_skip(src, bytes);
    return bytes;
}


void save_write_magic(struct save *save, enum save_magic value)
{
    save_write(save, &value, sizeof(value));

    if (unlikely(save->prof != NULL)) {
        struct save_prof *prof = save->prof;
        const size_t prev = prof->depth - 1;
        const size_t it = save_len(save);

        if (prof->depth && prof->stack[prev].magic == value) {
            prof->bytes[value] += it - prof->stack[prev].it;
            prof->depth--;
        }
        else {
            assert(prof->depth < array_len(prof->stack));
            prof->stack[prof->depth].magic = value;
            prof->stack[prof->depth].it = it - 1;
            prof->depth++;
        }
    }
}

bool save_read_magic(struct save *save, enum save_magic exp)
{
    enum save_magic value = 0;
    save_read(save, &value, sizeof(value));
    if (likely(value == exp)) return true;

    errf("invalid magic %x != %x", (unsigned) value, (unsigned) exp);
    return false;
}


void save_write_htable(struct save *save, const struct htable *ht)
{
    save_write_magic(save, save_magic_htable);
    save_write_value(save, (uint64_t) ht->len);

    for (const struct htable_bucket *it = htable_next(ht, NULL);
         it; it = htable_next(ht, it))
    {
        save_write_value(save, it->key);
        save_write_value(save, it->value);
    }

    save_write_magic(save, save_magic_htable);
}

bool save_read_htable(struct save *save, struct htable *ht)
{
    if (!save_read_magic(save, save_magic_htable)) return false;

    size_t len = save_read_type(save, uint64_t);
    htable_reset(ht);
    htable_reserve(ht, len);

    for (size_t i = 0; i < len; ++i) {
        struct htable_bucket bucket = {0};
        save_read_into(save, &bucket.key);
        save_read_into(save, &bucket.value);

        struct htable_ret ret = htable_put(ht, bucket.key, bucket.value);
        assert(ret.ok);
    }

    return save_read_magic(save, save_magic_htable);
}


void save_write_vec32(struct save *save, const struct vec32 *vec)
{
    save_write_magic(save, save_magic_vec32);

    if (!vec) {
        save_write_value(save, (typeof(vec->len)) 0);
        save_write_value(save, (typeof(vec->cap)) 0);
    }
    else {
        save_write_value(save, vec->len);
        save_write_value(save, vec->cap);
        save_write(save, vec->vals, vec->len * sizeof(vec->vals[0]));
    }

    save_write_magic(save, save_magic_vec32);
}

bool save_read_vec32(struct save *save, struct vec32 **ret)
{
    if (!save_read_magic(save, save_magic_vec32)) return false;

    struct vec32 *vec = *ret;
    typeof(vec->len) len = save_read_type(save, typeof(len));
    typeof(vec->cap) cap = save_read_type(save, typeof(cap));

    if (cap) vec = vec32_grow(vec, cap);
    else if (vec && !cap) { vec32_free(vec); vec = NULL; }

    if (vec) {
        vec->len = len;
        save_read(save, vec->vals, len * sizeof(vec->vals[0]));
    }

    *ret = vec;
    return save_read_magic(save, save_magic_vec32);
}


void save_write_vec64(struct save *save, const struct vec64 *vec)
{
    save_write_magic(save, save_magic_vec64);

    if (!vec) {
        save_write_value(save, (typeof(vec->len)) 0);
        save_write_value(save, (typeof(vec->cap)) 0);
    }
    else {
        save_write_value(save, vec->len);
        save_write_value(save, vec->cap);
        save_write(save, vec->vals, vec->len * sizeof(vec->vals[0]));
    }

    save_write_magic(save, save_magic_vec64);
}

bool save_read_vec64(struct save *save, struct vec64 **ret)
{
    if (!save_read_magic(save, save_magic_vec64)) return false;

    struct vec64 *vec = *ret;
    typeof(vec->len) len = save_read_type(save, typeof(len));
    typeof(vec->cap) cap = save_read_type(save, typeof(cap));

    if (cap) vec = vec64_grow(vec, cap);
    else if (vec && !cap) { vec64_free(vec); vec = NULL; }

    if (vec) {
        vec->len = len;
        save_read(save, vec->vals, len * sizeof(vec->vals[0]));
    }

    *ret = vec;
    return save_read_magic(save, save_magic_vec64);
}


// -----------------------------------------------------------------------------
// prof
// -----------------------------------------------------------------------------

void save_prof(struct save *save)
{
    if (!save->prof) save->prof = calloc(1, sizeof(*save->prof));
    else {
        memset(save->prof->bytes, 0, sizeof(save->prof->bytes));
        save->prof->depth = 0;
    }
}

void save_prof_dump(struct save *save)
{
    if (!save->prof) return;
    struct save_prof *prof = save->prof;

    char buffer[s_page_len] = {0};
    char *it = buffer;
    const char *end = it + sizeof(buffer);

    it += snprintf(it, end - it, "save: {total:");
    it += str_scaled(save_len(save), it, end - it);

    for (enum save_magic magic = 0; magic < save_magic_len; ++magic) {
        if (!prof->bytes[magic]) continue;

        const char *str = "";
        switch(magic) {
        case save_magic_vec64:  { str = "vec"; break; }
        case save_magic_ring32: { str = "rg3"; break; }
        case save_magic_ring64: { str = "rg6"; break; }
        case save_magic_htable: { str = " ht"; break; }
        case save_magic_symbol: { str = "sym"; break; }
        case save_magic_heap:   { str = " hp"; break; }

        case save_magic_sim:      { str = "sim"; break; }
        case save_magic_world:    { str = "wrd"; break; }
        case save_magic_star:     { str = "str"; break; }
        case save_magic_lab:      { str = "lab"; break; }
        case save_magic_tech:     { str = "tch"; break; }
        case save_magic_log:      { str = "log"; break; }
        case save_magic_lanes:    { str = "lns"; break; }
        case save_magic_lane:     { str = " ln"; break; }
        case save_magic_tape_set: { str = "tps"; break; }
        case save_magic_shards:   { str = "shd"; break; }

        case save_magic_atoms:  { str = "atm"; break; }
        case save_magic_mods:   { str = "mds"; break; }
        case save_magic_mod:    { str = "mod"; break; }
        case save_magic_chunks: { str = "cks"; break; }
        case save_magic_chunk:  { str = " ck"; break; }
        case save_magic_active: { str = "act"; break; }
        case save_magic_energy: { str = "nrg"; break; }

        case save_magic_state_world:   { str = "swd"; break; }
        case save_magic_state_chunk:   { str = "sck"; break; }

        default: { assert(false); }
        }

        it += snprintf(it, end - it, "} {%s:", str);
        it += str_scaled(prof->bytes[magic], it, end - it);
    }

    it += snprintf(it, end - it, "}\n");
    fprintf(stderr, "%s", buffer);

    free(save->prof);
    save->prof = NULL;
}
