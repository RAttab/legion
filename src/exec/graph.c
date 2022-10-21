/* graph.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/tape.h"
#include "db/items.h"
#include "db/tapes.h"
#include "items/config.h"
#include "utils/htable.h"
#include "render/render.h"

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

struct graph
{
    struct set *items;
    struct htable graph;
    char *first, *it, *end;
};

void graph_write(struct graph *graph, const char *str)
{
    size_t len = strnlen(str, graph->end - graph->it);
    memcpy(graph->it, str, len);
    graph->it += len;
}

legion_printf(2, 3) void graph_writef(struct graph *graph, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    graph->it += vsnprintf(graph->it, graph->end - graph->it, fmt, args);
    va_end(args);
}


void graph_graph(struct graph *graph)
{
    graph->items = set_alloc();
    struct set *inputs = set_alloc();
    struct set *outputs = set_alloc();
    htable_reserve(&graph->graph, 0xFF);

    for (enum item item = 0; item < items_max; ++item) {
        const struct tape *tape = tapes_get(item);
        if (!tape) continue;
        set_put(graph->items, item);

        size_t i = 0;
        for (struct tape_ret ret = tape_at(tape, i);
             ret.state != tape_eof; ++i, ret = tape_at(tape, i))
        {
            set_put(graph->items, ret.item);
            if (ret.state == tape_input) set_put(inputs, ret.item);
            if (ret.state == tape_output) set_put(outputs, ret.item);
        }

        for (enum item it = set_next(inputs, set_nil);
             it != set_nil; it = set_next(inputs, it))
        {
            struct htable_ret ret = htable_get(&graph->graph, it);

            struct set *edges = NULL;
            if (ret.ok) edges = (void *) ret.value;
            else {
                edges = set_alloc();
                ret = htable_put(&graph->graph, it, (uintptr_t) edges);
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

void graph_color(struct graph *graph, const char *color, enum item first, enum item last)
{
    graph_writef(graph, "subgraph { node [color=%s]; ", color);
    for (enum item it = first; it < last; ++it) {
        if (!set_test(graph->items, it)) continue;
        graph_writef(graph, "\"%02x\" ", it);
    }
    graph_write(graph, "}\n");
}

void graph_dot(struct graph *graph)
{
    enum { len = s_page_len * 10 };
    graph->first = calloc(len, sizeof(*graph->first));
    graph->end = graph->first + len;
    graph->it = graph->first;

    graph_write(graph, "strict digraph {\n");

    graph_color(graph, "blue",   items_natural_first, items_natural_last);
    graph_color(graph, "purple", items_synth_first,   items_synth_last);
    graph_color(graph, "green",  items_passive_first, items_passive_last);
    graph_color(graph, "red",    items_active_first,  items_logistics_last);

    char label[item_str_len] = {0};
    for (enum item it = 0; it < items_max; ++it) {
        if (!set_test(graph->items, it)) continue;
        item_str(it, label, sizeof(label));
        graph_writef(graph, "subgraph { \"%02x\" [label=\"%02x:%s\"] }\n", it, it, label);
    }

    for (const struct htable_bucket *it = htable_next(&graph->graph, NULL);
         it; it = htable_next(&graph->graph, it))
    {
        enum item in = it->key;
        graph_writef(graph, "\"%02x\" -> { ", in);

        struct set *outputs = (void *) it->value;
        for (enum item it = set_next(outputs, set_nil);
             it != set_nil; it = set_next(outputs, it))
        {
            graph_writef(graph, "\"%02x\" ", it);
        }

        graph_write(graph, "}\n");
    }

    graph_write(graph, "}\n");
}

void graph_output(struct graph *graph)
{
    write(1, graph->first, graph->it - graph->first);
}

bool graph_run(void)
{
    struct graph graph = {0};
    graph_graph(&graph);
    graph_dot(&graph);
    graph_output(&graph);
    return true;
}
