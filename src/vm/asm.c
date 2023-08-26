/* asm.c
   Rémi Attab (remi.attab@gmail.com), 02 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/asm.h"


// -----------------------------------------------------------------------------
// assembly
// -----------------------------------------------------------------------------

struct assembly
{
    struct { struct asm_line *list; size_t len, cap; } line;
    struct { struct asm_jmp *list; size_t len, cap; } jmp;
};

struct assembly *asm_alloc(void)
{
    return calloc(1, sizeof(struct assembly));
}

void asm_free(struct assembly *as)
{
    if (as->line.list) free(as->line.list);
    if (as->jmp.list) free(as->jmp.list);
    free(as);
}

void asm_reset(struct assembly *as)
{
    as->line.len = as->jmp.len = 0;
}

bool asm_empty(const struct assembly *as)
{
    return !as->line.len;
}

uint32_t asm_rows(const struct assembly *as)
{
    return as->line.len;
}

vm_ip asm_ip(const struct assembly *as, uint32_t row)
{
    assert(row < as->line.len);
    return as->line.list[row].ip;
}

uint32_t asm_row(const struct assembly *as, vm_ip ip)
{
    struct asm_line *it = as->line.list;
    struct asm_line *end = it + as->line.len;
    while (it < end - 1 && ip >= (it + 1)->ip) ++it;
    return it - as->line.list;
}

asm_it asm_at(const struct assembly *as, uint32_t row)
{
    assert(row <= as->line.len);
    return as->line.list + row;
}

struct asm_jmp_it asm_jmp_begin(
        const struct assembly *as, uint32_t min, uint32_t max)
{
    assert(min < max);
    assert(min < as->line.len);
    assert(max <= as->line.len);
    return (struct asm_jmp_it) { .it = UINT32_MAX, .min = min, .max = max };
}

bool asm_jmp_step(const struct assembly *as, struct asm_jmp_it *it)
{
    it->it = it->it == UINT32_MAX ? 0 : it->it + 1;

    for (; it->it < as->jmp.len; ++it->it) {
        const struct asm_jmp *jmp = as->jmp.list + it->it;

        uint32_t jmp_min = legion_min(jmp->src, jmp->dst);
        uint32_t jmp_max = legion_max(jmp->src, jmp->dst);
        if (it->min > jmp_max || it->max < jmp_min) continue;

        it->src = jmp->src;
        it->dst = jmp->dst;
        it->layer = jmp->layer;
        return true;
    }

    return false;
}

asm_jmp_it asm_jmp_from(const struct assembly *as, uint32_t row)
{
    for (size_t i = 0; i < as->jmp.len; ++i) {
        const struct asm_jmp *it = as->jmp.list + i;
        if (it->src == row) return it;
    }
    return nullptr;
}

asm_jmp_it asm_jmp_to(const struct assembly *as, uint32_t row)
{
    for (size_t i = 0; i < as->jmp.len; ++i) {
        const struct asm_jmp *it = as->jmp.list + i;
        if (it->dst == row) return it;
    }
    return nullptr;
}

static struct asm_line *asm_append_line(struct assembly *as)
{
    if (unlikely(as->line.len == as->line.cap)) {
        size_t old = legion_xchg(
                &as->line.cap,
                as->line.cap ? as->line.cap * 2 : 1024);

        as->line.list = realloc_zero(
                as->line.list,
                old, as->line.cap,
                sizeof(*as->line.list));
    }

    return as->line.list + as->line.len++;
}

static struct asm_jmp *asm_append_jmp(struct assembly *as)
{
    if (unlikely(as->jmp.len == as->jmp.cap)) {
        size_t old = legion_xchg(
                &as->jmp.cap,
                as->jmp.cap ? as->jmp.cap * 2 : 1024);

        as->jmp.list = realloc_zero(
                as->jmp.list,
                old, as->jmp.cap,
                sizeof(*as->jmp.list));
    }

    return as->jmp.list + as->jmp.len++;
}

void asm_parse(struct assembly *as, const struct mod *mod)
{
    as->line.len = as->jmp.len = 0;

    uint32_t row = 0;
    vm_ip prev = vm_ip_nil;

    for (const uint8_t *it = mod->code; it < mod->code + mod->len; row++) {
        struct asm_line *line = asm_append_line(as);

        line->row = row;
        line->ip = it - mod->code;

        line->op = *it; ++it;
        line->arg = vm_op_arg(line->op);

        size_t len = vm_op_arg_bytes(line->arg);
        assert(len <= sizeof(line->value));
        memcpy(&line->value, it, len);
        it += len;

        struct mod_index index = mod_index(mod, line->ip);
        if (index.len && index.ip != prev) {
            line->symbol = make_symbol_len(mod->src + index.pos, index.len);
            prev = index.ip;
        }
    }

    struct asm_line *const first = as->line.list;
    for (struct asm_line *it = first; it < first + as->line.len; ++it)
    {
        if (it->arg != vm_op_arg_off) continue;

        struct asm_jmp *jmp = asm_append_jmp(as);
        jmp->src = it - first;
        jmp->dst = asm_row(as, it->value);
        jmp->layer = UINT8_MAX;
    }

    int cmp_min(const void *l, const void *r)
    {
        const struct asm_jmp *lhs = l;
        const struct asm_jmp *rhs = r;

        uint32_t lhs_min = legion_min(lhs->src, lhs->dst);
        uint32_t rhs_min = legion_min(rhs->src, rhs->dst);

        return
            lhs_min < rhs_min ? -1 :
            lhs_min > rhs_min ? +1 :
            0;
    }
    qsort(as->jmp.list, as->jmp.len, sizeof(*as->jmp.list), &cmp_min);

    int cmp_delta(const void *l, const void *r)
    {
        const struct asm_jmp *const *lhs = l;
        const struct asm_jmp *const *rhs = r;

        uint32_t lhs_delta =
            legion_max((*lhs)->src, (*lhs)->dst) -
            legion_min((*lhs)->src, (*lhs)->dst);

        uint32_t rhs_delta =
            legion_max((*rhs)->src, (*rhs)->dst) -
            legion_min((*rhs)->src, (*rhs)->dst);

        return
            lhs_delta > rhs_delta ? +1 :
            lhs_delta < rhs_delta ? -1 :
            0;
    }

    struct asm_jmp **delta = calloc(as->jmp.len, sizeof(*delta));
    for (size_t i = 0; i < as->jmp.len; ++i) delta[i] = as->jmp.list + i;
    qsort(delta, as->jmp.len, sizeof(*delta), &cmp_delta);

    for (size_t i = 0; i < as->jmp.len; ++i) {
        struct asm_jmp *jmp = delta[i];
        jmp->layer = 0;

        uint32_t min = legion_min(jmp->src, jmp->dst);
        uint32_t max = legion_max(jmp->src, jmp->dst);

        for (size_t i = 0; i < as->jmp.len; ++i) {
            struct asm_jmp *it = as->jmp.list + i;
            if (likely(jmp->layer > it->layer)) continue;
            if (it->layer == UINT8_MAX || it == jmp) continue;

            uint32_t it_min = legion_min(it->src, it->dst);
            uint32_t it_max = legion_max(it->src, it->dst);
            if (min >= it_max || max <= it_min) continue;

            jmp->layer = legion_max(jmp->layer, it->layer + 1);
        }
    }
}

// -----------------------------------------------------------------------------
// dump
// -----------------------------------------------------------------------------

void asm_dump(struct assembly *as)
{
    dbgf("asm.line: %zu:%zu", as->line.len, as->line.cap);
    for (size_t i = 0; i < as->line.len; ++i) {
        const struct asm_line *line = as->line.list + i;

        char arg[6] = {0};
        size_t arg_len = vm_op_arg_fmt(line->arg, line->value, arg, sizeof(arg));

        dbgf("  [%04u] ip=%08x op=%02x:%6s arg=%02x value=%6.*s sym=%02u:%.*s",
                line->row, line->ip,
                line->op, vm_op_str(line->op), line->arg,
                (unsigned) arg_len, arg,
                line->symbol.len, (unsigned) line->symbol.len, line->symbol.c);
    }

    dbgf("asm.jmp: %zu:%zu", as->jmp.len, as->jmp.cap);
    for (size_t i = 0; i < as->jmp.len; ++i) {
        const struct asm_jmp *jmp = as->jmp.list + i;
        const struct asm_line *src = as->line.list + jmp->src;
        const struct asm_line *dst = as->line.list + jmp->dst;

        dbgf("  [%04zu] layer=%02u src=%04u:%08x dst=%04u:%08x",
                i, jmp->layer, jmp->src, src->ip, jmp->dst, dst->ip);
    }
}