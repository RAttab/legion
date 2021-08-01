/* viz.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/tape.h"
#include "game/item.h"
#include "utils/htable.h"

#include <stdarg.h>
#include <unistd.h>


// -----------------------------------------------------------------------------
// set
// -----------------------------------------------------------------------------

struct set { uint64_t data[4]; };

struct set *set_alloc(void) { return calloc(1, sizeof(struct set)); }

void set_reset(struct set *set)
{
    memset(set, 0, sizeof(*set));
}

void set_put(struct set *set, enum item i)
{
    assert(i < 0xFF);
    set->data[i / 64] |= 1ULL << (i % 64);
}

bool set_test(struct set *set, enum item i)
{
    assert(i < 0xFF);
    return set->data[i / 64] & 1ULL << (i % 64);
}

static enum item set_nil = 0xFF;
size_t set_next(struct set *set, enum item i)
{
    i = i == set_nil ? 0 : i+1;

    for (; i < set_nil; ++i) {
        if (set_test(set, i)) return i;
    }

    return set_nil;
}

void set_union(struct set *lhs, struct set *rhs)
{
    for (size_t i = 0; i < 4; ++i)
        lhs->data[i] |= rhs->data[i];
}


// -----------------------------------------------------------------------------
// graph
// -----------------------------------------------------------------------------

struct viz
{
    struct set *items;
    struct htable graph;
    char *first, *it, *end;
};

void viz_write(struct viz *viz, const char *str)
{
    size_t len = strnlen(str, viz->end - viz->it);
    memcpy(viz->it, str, len);
    viz->it += len;
}

legion_printf(2, 3) void viz_writef(struct viz *viz, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    viz->it += vsnprintf(viz->it, viz->end - viz->it, fmt, args);
    va_end(args);
}


void viz_graph(struct viz *viz)
{
    viz->items = set_alloc();
    struct set *inputs = set_alloc();
    struct set *outputs = set_alloc();
    htable_reserve(&viz->graph, 0xFF);

    for (enum item item = 0; item < 0xFF; ++item) {
        const struct tape *tape = tapes_get(item);
        if (!tape) continue;
        set_put(viz->items, item);

        size_t i = 0;
        for (struct tape_ret ret = tape_at(tape, i);
             ret.state != tape_eof; ++i, ret = tape_at(tape, i))
        {
            set_put(viz->items, ret.item);
            if (ret.state == tape_input) set_put(inputs, ret.item);
            if (ret.state == tape_output) set_put(outputs, ret.item);
        }

        for (enum item it = set_next(inputs, set_nil);
             it != set_nil; it = set_next(inputs, it))
        {
            struct htable_ret ret = htable_get(&viz->graph, it);

            struct set *edges = NULL;
            if (ret.ok) edges = (void *) ret.value;
            else {
                edges = set_alloc();
                ret = htable_put(&viz->graph, it, (uintptr_t) edges);
                assert(ret.ok);
            }

            set_union(edges, outputs);
        }

        set_reset(inputs);
        set_reset(outputs);
    }
    free(inputs);
    free(outputs);
}

void viz_color(struct viz *viz, const char *color, enum item first, enum item last)
{
    viz_writef(viz, "subgraph { node [color=%s]; ", color);
    for (enum item it = first; it < last; ++it) {
        if (!set_test(viz->items, it)) continue;
        viz_writef(viz, "\"%02x\" ", it);
    }
    viz_write(viz, "}\n");
}

void viz_dot(struct viz *viz)
{
    enum { len = s_page_len * 10 };
    viz->first = calloc(len, sizeof(*viz->first));
    viz->end = viz->first + len;
    viz->it = viz->first;

    viz_write(viz, "strict digraph {\n");

    viz_color(viz, "blue", ITEM_NATURAL_FIRST, ITEM_SYNTH_FIRST);
    viz_color(viz, "green", ITEM_PASSIVE_FIRST, ITEM_PASSIVE_LAST);
    viz_color(viz, "red", ITEM_LOGISTICS_FIRST, ITEM_ACTIVE_LAST);

    char label[item_str_len] = {0};
    for (enum item it = 0; it < ITEM_MAX; ++it) {
        if (!set_test(viz->items, it)) continue;
        item_str(it, label, sizeof(label));
        viz_writef(viz, "subgraph { \"%02x\" [label=\"%02x:%s\"] }\n", it, it, label);
    }

    for (struct htable_bucket *it = htable_next(&viz->graph, NULL);
         it; it = htable_next(&viz->graph, it))
    {
        enum item in = it->key;
        viz_writef(viz, "\"%02x\" -> { ", in);

        struct set *outputs = (void *) it->value;
        for (enum item it = set_next(outputs, set_nil);
             it != set_nil; it = set_next(outputs, it))
        {
            viz_writef(viz, "\"%02x\" ", it);
        }

        viz_write(viz, "}\n");
    }

    viz_write(viz, "}\n");
}

void viz_output(struct viz *viz)
{
    write(1, viz->first, viz->it - viz->first);
}

int viz_run(int argc, char **argv)
{
    (void) argc, (void) argv;
    tapes_populate();

    struct viz viz = {0};
    viz_graph(&viz);
    viz_dot(&viz);
    viz_output(&viz);

    return 0;
}
