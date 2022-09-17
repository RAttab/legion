/* bits.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/hash.h"

struct save;


// -----------------------------------------------------------------------------
// builtins
// -----------------------------------------------------------------------------

inline size_t u64_pop(uint64_t x) { return __builtin_popcountl(x); }

inline size_t u32_clz(uint32_t x) { return likely(x) ? __builtin_clz(x) : 32; }
inline size_t u64_clz(uint64_t x) { return likely(x) ? __builtin_clzl(x) : 64; }
inline size_t u64_log2(uint64_t x) { return likely(x) ? 63 - u64_clz(x) : 0; }

inline size_t u64_ctz(uint64_t x) { return likely(x) ? __builtin_ctzl(x) : 64; }


// -----------------------------------------------------------------------------
// math
// -----------------------------------------------------------------------------

inline uint32_t u64_top(uint64_t x) { return x >> 32; }
inline uint32_t u64_bot(uint64_t x) { return x << 32 >> 32; }

inline int64_t i64_ceil_div(int64_t x, int64_t d)
{

    return (d && x) ? (x - 1) / d + 1 : 1;
}

inline int64_t u64_ceil_div(uint64_t x, uint64_t d)
{
    return (d && x) ? (x - 1) / d + 1 : 1;
}

inline int64_t i64_clamp(int64_t x, int64_t min, int64_t max)
{
    return legion_min(legion_max(x, min), max);
}

inline size_t align_cache(size_t sz)
{
    return sz ? ((((sz - 1) >> 6) + 1) << 6) : 0;
}

inline uint16_t u16_saturate_add(uint64_t val, uint64_t add)
{
    val += add;
    return val > UINT16_MAX ? UINT16_MAX : val;
}

inline uint32_t u32_saturate_add(uint64_t val, uint64_t add)
{
    val += add;
    return val > UINT32_MAX ? UINT32_MAX : val;
}


// -----------------------------------------------------------------------------
// bits
// -----------------------------------------------------------------------------

struct bits
{
    size_t len;
    uint64_t bits;
};


inline bool bits_inline(const struct bits *bits)
{
    return bits->len <= 64;
}

inline uint64_t *bits_array(struct bits *bits)
{
    return bits_inline(bits) ? &bits->bits : (uint64_t *) bits->bits;
}

inline const uint64_t *bits_array_c(const struct bits *bits)
{
    return bits_inline(bits) ? &bits->bits : (const uint64_t *) bits->bits;
}


inline void bits_init(struct bits *bits)
{
    bits->len = 0;
    bits->bits = 0;
}

inline void bits_free(struct bits *bits)
{
    if (!bits_inline(bits)) free(bits);
}

void bits_grow(struct bits *, size_t);
bool bits_load(struct bits *, struct save *);
void bits_save(const struct bits *, struct save *);
hash_val bits_hash(struct bits *, hash_val);


inline void bits_reset(struct bits *bits)
{
    bits_free(bits);
    bits->len = 0;
    bits->bits = 0;
}

inline void bits_clear(struct bits *bits)
{
    memset(bits_array(bits), 0, u64_ceil_div(bits->len, 64) * 8);
}

inline void bits_copy(struct bits *dst, const struct bits *src)
{
    bits_grow(dst, src->len);
    memcpy(bits_array(dst), bits_array_c(src), u64_ceil_div(src->len, 64) * 8);
}

inline bool bits_test(const struct bits *bits, size_t index)
{
    assert(index < bits->len);
    return bits_array_c(bits)[index / 64] & 1ULL << (index % 64);
}

inline void bits_set(struct bits *bits, size_t index)
{
    assert(index < bits->len);
    bits_array(bits)[index / 64] |= 1ULL << (index % 64);
}

inline void bits_unset(struct bits *bits, size_t index)
{
    assert(index < bits->len);
    bits_array(bits)[index / 64] &= ~(1ULL << (index % 64));
}

inline void bits_flip(struct bits *bits)
{
    uint64_t *end = bits_array(bits) + u64_ceil_div(bits->len, 64);
    for (uint64_t *it = bits_array(bits); it < end; ++it)
        *it = ~*it;
}

inline void bits_minus(struct bits *lhs, const struct bits *rhs)
{
    size_t end = u64_ceil_div(legion_min(lhs->len, rhs->len), 64);
    for (size_t i = 0; i < end; ++i) {
        uint64_t *dst = bits_array(lhs) + i;
        *dst &= ~(*dst & bits_array_c(rhs)[i]);
    }
}

inline size_t bits_next(const struct bits *bits, size_t start)
{
    assert(start <= bits->len);
    if (start == bits->len) return bits->len;

    uint64_t mask = ~((1ULL << (start % 64)) - 1);
    const uint64_t *it = bits_array_c(bits) + (start / 64);
    const uint64_t *end = bits_array_c(bits) + u64_ceil_div(bits->len, 64);

    for (size_t i = start / 64; it < end; i++, it++) {
        size_t bit = u64_ctz(*it & mask);
        if (bit < 64) return i * 64 + bit;
        mask = -1ULL;
    }

    return bits->len;
}
