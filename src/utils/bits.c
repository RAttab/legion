/* bits.c
   Rémi Attab (remi.attab@gmail.com), 14 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/bits.h"
#include "utils/hash.h"
#include "utils/save.h"

// -----------------------------------------------------------------------------
// bits
// -----------------------------------------------------------------------------

void bits_grow(struct bits *bits, size_t len)
{
    if (likely(len < bits->len)) return;
    if (likely(len <= 64)) { bits->len = len; return; }

    size_t new = u64_ceil_div(len, 64);
    size_t old = u64_ceil_div(bits->len, 64);
    if (likely(new <= old)) { bits->len = len; return; }

    if (!bits_inline(bits)) {
        bits->len = len;
        bits->bits = (uintptr_t) mem_array_realloc_t(
                bits_array(bits), bits->bits, old, new);
    }
    else {
        uint64_t copy = bits->bits;
        bits->bits = (uintptr_t) mem_array_alloc_t(bits->bits, new);

        bits->len = len;
        bits_array(bits)[0] = copy;
    }
}

bool bits_load(struct bits *bits, struct save *save)
{
    if (!save_read_magic(save, save_magic_bits)) return false;

    typeof(bits->len) len = save_read_type(save, typeof(len));
    bits_grow(bits, len);

    uint64_t *end = bits_array(bits) + u64_ceil_div(len, 64);
    for (uint64_t *it = bits_array(bits); it < end; ++it)
        save_read_into(save, it);

    return save_read_magic(save, save_magic_bits);
}

void bits_save(const struct bits *bits, struct save *save)
{
    save_write_magic(save, save_magic_bits);

    save_write_value(save, bits->len);

    const uint64_t *end = bits_array_c(bits) + u64_ceil_div(bits->len, 64);
    for (const uint64_t *it = bits_array_c(bits); it < end; ++it)
        save_write_value(save, *it);

    save_write_magic(save, save_magic_bits);
}

hash_val bits_hash(const struct bits *bits, hash_val hash)
{
    hash = hash_value(hash, bits->len);

    const uint64_t *end = bits_array_c(bits) + u64_ceil_div(bits->len, 64);
    for (const uint64_t *it = bits_array_c(bits); it < end; ++it)
        hash = hash_value(hash, *it);

    return hash;

}

size_t bits_dump(const struct bits *bits, char *start, size_t len)
{
    char *it = start;
    char *const end = it + len;

    it += snprintf(it, end - it, "{ ");

    for (uint32_t ix = bits_next(bits, 0);
         ix < bits->len; ix = bits_next(bits, ix + 1))
        it += snprintf(it, end - it, "%02x ", ix);

    it += snprintf(it, end - it, "}");
    return it - start;
}
