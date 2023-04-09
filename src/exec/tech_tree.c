/* tech_tree.c
   Rémi Attab (remi.attab@gmail.com), 25 Mar 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef uint8_t node_id;
static const node_id node_id_max = UINT8_MAX;

inline uint8_t node_id_layer(node_id id) { return id / index_cap; }
inline uint8_t node_id_index(node_id id) { return id % index_cap; }
inline node_id make_node_id(uint8_t layer, uint8_t index)
{
    return layer * index_cap + index;
}

inline node_id node_id_first(uint8_t layer) { return make_node_id(layer, 0); }
inline node_id node_id_last(uint8_t layer) { return make_node_id(layer, index_cap); }

// -----------------------------------------------------------------------------
// edge_list
// -----------------------------------------------------------------------------

struct legion_packed edge { node_id id; uint32_t count; };

struct legion_packed edges
{
    uint8_t len;
    struct edge list[edges_cap];
};

static void edges_init(struct edges *edges) { memset(edges, 0, sizeof(*edges)); }

static void edges_copy(struct edges *dst, const struct edges *src)
{
    memcpy(dst, src, sizeof(*src));
}

static struct edge *edges_find(struct edges *edges, node_id id)
{
    struct edge *it = edges->list;
    struct edge *const end = it + edges->len;
    for (; it < end && it->id < id; ++it);
    return it->id == id ? it : NULL;
}

static uint32_t edges_count(struct edges *edges, node_id id)
{
    struct edge *it = edges_find(edges, id);
    return it ? it->count : 0;
}

static size_t edges_inc(struct edges *edges, node_id id, uint32_t count)
{
    assert(edges->len < edges_cap);

    struct edge *edge = edges->list;
    struct edge *const end = edge + edges->len;
    for (; edge < end && edge->id < id; ++edge);

    if (edge->id == id) edge->count += count;
    else {
        for (struct edge *it = end; it > edge; --it) *it = *(it - 1);
        *edge = (struct edge) { .id = id, .count = count };
        edges->len++;
    }

    return edge - edges->list;
}


static size_t edges_dec_at(struct edges *edges, struct edge *it, size_t count)
{
    struct edge *const end = it + edges->len;
    assert(it < end);

    it->count -= legion_min(count, it->count);
    if (it->count) return it->count;

    for (; it < end - 1; ++it) *it = *(it + 1);
    edges->len--;

    return 0;
}

static size_t edges_dec(struct edges *edges, node_id id, uint32_t count)
{
    struct edge *it = edges->list;
    const struct edge *end = it + edges->len;
    for (; it < end && it->id < id; ++it);

    return it->id == id ? edges_dec_at(edges, it, count) : 0;
}


// -----------------------------------------------------------------------------
// node
// -----------------------------------------------------------------------------

struct legion_packed node
{
    node_id id;
    enum im_type type;
    bool generated, done;

    struct symbol name, syllable, config;

    struct { uint32_t node, total; } energy;
    struct { uint32_t node, min, total; } work;

    struct { uint8_t len; char *data; } specs;
    struct { node_id id; struct symbol name; } host;

    struct { uint8_t bits, work; uint16_t energy; } lab;
    struct { struct bits set; struct edges edges; } children, needs;
    struct { struct edges in, needs; } base;
    struct edges out;
};

static void node_dump(const struct node *node, const char *title)
{
    char type = 0;
    switch (node->type) {
    case im_type_natural: { type = 'n'; break; }
    case im_type_synthetic: { type = 's'; break; }
    case im_type_active: { type = 'a'; break; }
    case im_type_passive: { type = 'p'; break; }
    case im_type_logistics: { type = 'l'; break; }
    case im_type_sys: { type = 'y'; break; }
    default: { type = '?'; break; }
    }

    char child_edges_str[256] = {0};
    {
        char *it = child_edges_str;
        char *const end = child_edges_str + sizeof(child_edges_str);

        it += snprintf(it, end - it, "%u:{", node->children.edges.len);
        for (size_t i = 0; i < node->children.edges.len; ++i) {
            const struct edge *edge = node->children.edges.list + i;
            it += snprintf(it, end - it, " %02x:%u", edge->id, edge->count);
        }
        it += snprintf(it, end - it, " }");
    }

    char needs_edges_str[256] = {0};
    {
        char *it = needs_edges_str;
        char *const end = needs_edges_str + sizeof(needs_edges_str);

        it += snprintf(it, end - it, "%u:{", node->needs.edges.len);
        for (size_t i = 0; i < node->needs.edges.len; ++i) {
            const struct edge *edge = node->needs.edges.list + i;
            it += snprintf(it, end - it, " %02x:%u", edge->id, edge->count);
        }
        it += snprintf(it, end - it, " }");
    }

    infof("%s: id=%02x:%s:%c, child=%s, needs=%s, work=%u/%u/%u, en=%u/%u",
            title, node->id, node->name.c, type,
            child_edges_str, needs_edges_str,
            node->work.node, node->work.min, node->work.total,
            node->energy.node, node->energy.total);
}

static void node_child_inc(struct node *node, node_id id, size_t count)
{
    edges_inc(&node->children.edges, id, count);
    bits_put(&node->children.set, id);
}

static void node_needs_inc(struct node *node, node_id id, size_t count)
{
    edges_inc(&node->needs.edges, id, count);
    bits_put(&node->needs.set, id);
}

static void node_needs_dec(struct node *node, node_id id, size_t count)
{
    if (!edges_dec(&node->needs.edges, id, count))
        bits_del(&node->needs.set, id);
}

// -----------------------------------------------------------------------------
// tree
// -----------------------------------------------------------------------------

struct tree
{
    struct node *nodes;
    struct htable symbols;
    struct { node_id printer, assembly; } ids;
};

static struct tree tree_init(void)
{
    struct tree tree = {0};

    const size_t len = sizeof(struct node) * layer_cap * index_cap;
    const int flags = MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE;
    tree.nodes = mmap(NULL, len, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (tree.nodes == MAP_FAILED)
        failf_errno("unable to mmap tree of len='%lx'", len);
    htable_reset(&tree.symbols);
    return tree;
}

static struct node *tree_node(struct tree *tree, node_id id)
{
    struct node *node = tree->nodes + id;
    return node->id ? node : NULL;
}

static struct node *tree_symbol(struct tree *tree, const struct symbol *sym)
{
    struct htable_ret ret = htable_get(&tree->symbols, symbol_hash(sym));
    return ret.ok ? tree_node(tree, ret.value) : NULL;
}

static struct node *tree_append(struct tree *tree, uint8_t layer)
{
    struct node *first = tree->nodes + node_id_last(layer);
    struct node *last = first - index_cap;
    if (last == tree->nodes) last++;
    first--;

    struct node *node = NULL;
    for (struct node *it = first; it >= last; --it) {
        if (!it->id) { node = it; break; }
    }
    if (!node) return NULL;

    node->id = node - tree->nodes;
    return node;
}

static struct node *tree_insert(
        struct tree *tree, uint8_t layer, const struct symbol *sym)
{
    struct node *first = tree->nodes + node_id_first(layer);
    struct node *const last = first + index_cap;
    if (first == tree->nodes) first++;

    struct node *node = NULL;
    for (struct node *it = first; it < last; ++it) {
        if (!it->id) { node = it; break; }
    }
    if (!node) return NULL;

    node->name = *sym;
    node->id = node - tree->nodes;

    hash_val hash = symbol_hash(sym);
    struct htable_ret ret = htable_put(&tree->symbols, hash, node->id);
    assert(ret.ok);

    if (hash == symbol_hash_c("printer")) tree->ids.printer = node->id;
    if (hash == symbol_hash_c("assembly")) tree->ids.assembly = node->id;

    return node;
}

static bool tree_set_symbol(
        struct tree *tree, struct node *node, const struct symbol *sym)
{
    struct htable_ret ret = htable_put(&tree->symbols, symbol_hash(sym), node->id);
    return ret.ok;
}

static struct symbol tree_name(struct tree *tree, node_id id)
{
    const struct node *node = tree_node(tree, id);
    return node ? node->name : make_symbol("nil");
}
