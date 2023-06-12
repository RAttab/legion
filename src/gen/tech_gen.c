/* tech_gen.c
   Rémi Attab (remi.attab@gmail.com), 21 Feb 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

struct gen
{
    struct tree *tree;
    struct node *node;
    struct rng rng;
    uint32_t threshold;
};

static void gen_node_name(struct gen *gen)
{
    static const char *heads[] = {
        "mono", "duo", "tri", "tetra", "penta", "hexa", "hepta", "octo", "ennea",
        "deca", "hendeca", "dodeca", "decatria", "decatessara", "decapente"
    };
    static const char *tails[] = { // sizeof <= 4
        "alm", "alt", "ate", "ex", "gen", "itil", "ide", "ium", "ols", "on", "oid",
        "ry", "sh", "tor" };

    struct node *node = gen->node;


    struct symbol name = {0};

    void append(const char *src)
    {
        char *dst = name.c + name.len;

        if (str_is_vowel(*src) && str_is_vowel(*dst))
            dst--;

        for (; *src && name.len < name_cap; ++dst, ++src, ++name.len)
            *dst = *src;
    }

    void append_syllable(size_t ix) {
        assert(ix < node->needs.edges.len);
        const struct node *elem = tree_node(gen->tree, node->needs.edges.list[ix].id);
        assert(elem);
        const struct symbol *syllable = &elem->syllable;
        append(syllable->c);
    }

    void append_tail(void) {
        append(tails[rng_uni(&gen->rng, 0, array_len(tails))]);
    }

    size_t rng_next(size_t max) {
        return max ? rng_uni(&gen->rng, 0, max) : 0;
    }

    for (size_t attempt = 0; attempt < 10; ++attempt) {
        memset(&name, 0, sizeof(name));

        size_t head = node_id_layer(node->id) - 1;
        append(heads[head]);

        size_t syllables = rng_uni(&gen->rng, 2, 3);
        size_t ix = node->needs.edges.len - 1;
        do {
            if (syllables-- == 0) {
                append_tail();
                *(name.c + name.len) = '-'; name.len++;
                append(heads[head = rng_next(head)]);
                syllables = rng_uni(&gen->rng, 1, 2);
            }

            append_syllable(ix);
            ix = rng_next(ix);
        } while (ix && name.len + 3 + 4 < name_cap);

        append_tail();
        
        if (tree_set_symbol(gen->tree, node, &name)) {
            node->name = name;
            return;
        }
    }

    assert(false);
}

static void gen_threshold(struct gen *gen)
{
    struct node *node = gen->node;

    struct edge *max = NULL;
    for (size_t i = 0; i < node->needs.edges.len; ++i) {
        struct edge *need = node->needs.edges.list + i;
        if (max && max->count > need->count) continue;
        max = need;
    }

    if (!max) return;

    gen->threshold = (((uint64_t) max->count) * 35) / 100;
    infof("gen.threshold: edge=%02x:%u, thresh=%u",
            max->id, max->count, gen->threshold);
}

// Prevents creating nodes with small amounts of elements.
static bool gen_trim_needs(struct gen *gen)
{
    struct node *node = gen->node;

    const struct edge *it = node->needs.edges.list;
    const struct edge *end = it + node->needs.edges.len;
    const struct edge *base = node->base.needs.list;

    bool trimmed = false;
    while (it < end) {
        while (base->id < it->id) base++;
        assert(base->id == it->id);

        if (base->count == it->count || it->count > gen->threshold) {
            it++;
            continue;
        }

        infof("gen.needs.trim: edge=%02x:%u, threshold=%u",
                it->id, it->count, gen->threshold);

        node_needs_dec(node, it->id, it->count);
        end = node->needs.edges.list + node->needs.edges.len;
        trimmed = true;
    }

    return trimmed;
}

// If our needs is right above the layer of its element then link it directly as
// a child.
static void gen_child_elem(struct gen *gen)
{
    struct node *node = gen->node;
    const uint8_t layer = node_id_layer(node->id);

    struct edge *it = node->needs.edges.list;
    const struct edge *end = it + node->needs.edges.len;

    while (it < end) {
        assert(layer > node_id_layer(it->id));
        if (node_id_layer(it->id) < layer - 1) { it++; continue; }

        node_child_inc(node, it->id, legion_min(it->count, child_count_cap));
        node_needs_dec(node, it->id, it->count);
        end = node->needs.edges.list + node->needs.edges.len;
    }

    if (node->needs.edges.len == 1) {
        struct edge *edge = node->needs.edges.list;
        node_child_inc(node, edge->id, legion_min(edge->count, child_count_cap));
        node_needs_dec(node, edge->id, edge->count);
    }
}

struct gen_count { uint32_t count, set, msb; };

struct gen_count gen_child_count(const struct node *node, const struct node *child)
{
    if (!bits_contains(&node->needs.set, &child->needs.set))
        return (struct gen_count) { .count = 0, .set = 0 };

    const struct edge *n = node->needs.edges.list;
    const struct edge *c = child->needs.edges.list;
    const struct edge *end = c + child->needs.edges.len;
    struct gen_count ret = { .count = UINT32_MAX, .set = 0, .msb = 0 };

    for (; c < end && ret.count > 0; c++) {
        while (n->id < c->id) n++;
        if (n->id != c->id) break;

        ret.set++;
        ret.msb = c->id;
        ret.count = legion_min(ret.count, n->count / c->count);
    }

    ret.count = legion_min(ret.count, child_count_cap);
    return ret;
}

static void gen_child_link(struct node *node, const struct node *child, size_t count)
{
    for (size_t i = 0; i < child->needs.edges.len; ++i) {
        const struct edge *needs = child->needs.edges.list + i;
        node_needs_dec(node, needs->id, needs->count * count);
    }

    node_child_inc(node, child->id, count);
    node_dump(node, "gen.child.link");
}

static bool gen_child_create(struct gen *gen, size_t layer)
{
    struct node *node = gen->node;

    {
        char str[256];
        bits_dump(&node->needs.set, str, sizeof(str));
        infof("gen.child.set: %s", str);
    }

    struct node *child = tree_append(gen->tree, layer);
    if (!child) return false;

    child->type = im_type_passive;
    child->generated = true;

    uint32_t max = 0;
    for (size_t i = 0; i < node->needs.edges.len; ++i)
        max = legion_max(max, node->needs.edges.list[i].count);

    const size_t div = 10;
    uint32_t min = UINT32_MAX;

    for (size_t i = 0; i < node->needs.edges.len; ++i) {
        struct edge *needs = node->needs.edges.list + i;

        uint32_t mult = rng_exp(&gen->rng, 1, div);
        uint32_t count = (max * mult) / div;
        count = legion_max(count, legion_max(1U, gen->threshold));
        count = legion_min(count, needs->count);

        if (node_id_layer(child->id) - 1 > node_id_layer(needs->id))
            node_needs_inc(child, needs->id, count);
        else {
            count = legion_min(count, child_count_cap);
            node_child_inc(child, needs->id, count);
        }

        min = legion_min(min, needs->count / count);
    }

    for (size_t i = 0; i < child->needs.edges.len; ++i) {
        struct edge *needs = child->needs.edges.list + i;
        node_needs_dec(node, needs->id, needs->count * min);
    }
    for (size_t i = 0; i < child->children.edges.len; ++i) {
        struct edge *needs = child->children.edges.list + i;
        node_needs_dec(node, needs->id, needs->count * min);
    }
    node_child_inc(node, child->id, min);

    node_dump(child, "gen.child.new");
    node_dump(node, "gen.child.create");
    return true;
}

static void gen_children(struct gen *gen)
{
    struct node *node = gen->node;
    if (!node->needs.edges.len) return;

    const uint8_t top = node_id_layer(node->id) - 1;

    uint8_t max = 0;
    for (size_t i = 0; i < node->children.edges.len; ++i)
        max = legion_max(max, node_id_layer(node->children.edges.list[i].id));

    // Ensures that our first child is: in the layer directly below our node to
    // ensure that we have the right depth; that it uses our MSB element so that
    // any newly added elements get added into the tree.
    if (max < top) {
        struct gen_count match = {0};
        const struct node *child = NULL;

        const struct node *first = gen->tree->nodes + node_id_last(top) - 1;
        const struct node *last = gen->tree->nodes + node_id_first(top) - 1;

        for (const struct node *it = first; it > last; --it) {
            if (!it->id) continue;
            if (im_type_elem(it->type)) continue;
            if (node->type == im_type_passive && it->type != im_type_passive)
                continue;

            struct gen_count counts = gen_child_count(node, it);
            if (!counts.count) continue;
            if (counts.msb < match.msb) continue;
            match = counts;
            child = it;
        }

        if (match.msb != bits_msb(&node->needs.set))
            gen_child_create(gen, top);
        else gen_child_link(node, child, match.count);
    }

    // Link to as many nodes in the tree as possible.
    while (node->needs.edges.len) {
        const struct node *child = NULL;
        struct gen_count match = { .count = 0, .set = 0, .msb = 0 };

        const uint8_t bottom = node_id_layer(bits_msb(&node->needs.set));
        assert(top > bottom);

        const struct node *first = gen->tree->nodes + node_id_last(top) - 1;
        const struct node *last = gen->tree->nodes + node_id_first(bottom + 1) - 1;

        for (const struct node *it = first; it > last; --it) {
            if (!it->id) continue;
            if (im_type_elem(it->type)) continue;
            if (node->type == im_type_passive && it->type != im_type_passive)
                continue;

            if (bits_has(&node->children.set, it->id)) continue;

            struct gen_count counts = gen_child_count(node, it);
            if (!counts.count) continue;
            if (counts.msb < match.msb) continue;
            if (counts.set < match.set) continue;

            match = counts;
            child = it;
        }

        if (child) gen_child_link(node, child, match.count);
        else if (!gen_trim_needs(gen)) break;
    }

    // If all else fails create new nodes to drain the rest of our needs.
    for (size_t failures = 0; node->needs.edges.len; gen_trim_needs(gen)) {
        const uint8_t bottom = node_id_layer(bits_msb(&node->needs.set));
        assert(top > bottom);

        bool ok = gen_child_create(gen, rng_exp(&gen->rng, bottom, top) + 1);
        if (!ok && ++failures > 5) break;
    }
}

static void gen_host(struct gen *gen)
{
    struct node *node = gen->node;

    if (node->host.name.len) {
        struct node *host = tree_symbol(gen->tree, &node->host.name);
        if (host) {
            node->host.id = host->id;
            return;
        }
    }

    for (size_t i = 0; i < node->children.edges.len; ++i) {
        struct node *child = tree_node(gen->tree, node->children.edges.list[i].id);
        if (im_type_elem(child->type)) {
            node->host.id = gen->tree->ids.printer;
            return;
        }
    }

    node->host.id = gen->tree->ids.assembly;
}

static void gen_lab(struct gen *gen)
{
    struct node *node = gen->node;

    uint64_t fuzz(uint64_t value)
    {
        const uint64_t half = value / 2;
        return rng_uni(&gen->rng,
                legion_max(1U, value - half),
                legion_max(2U, value + half));
    }

    uint8_t layer = node_id_layer(node->id);
    node->lab.bits = fuzz((layer * 64) / layer_cap);
    node->lab.work = fuzz((layer * UINT8_MAX) / layer_cap);
    node->lab.energy = fuzz(1ULL << layer);
}

static void gen_update(struct gen *gen)
{
    struct node *node = gen->node;

    // We don't want to update the needs of elems.
    if (im_type_elem(node->type)) {
        node->work.min += node->work.node;
        node->work.total += node->work.node;
        node->energy.total += node->energy.node * node->work.node;
        return;
    }

    size_t tape_len = 1;
    uint32_t child_work_max = 0;
    uint32_t child_energy_max = 0;

    edges_init(&node->needs.edges);
    bits_reset(&node->needs.set);

    for (size_t i = 0; i < node->children.edges.len; ++i) {
        struct edge *edge = node->children.edges.list + i;
        struct node *child = tree_node(gen->tree, edge->id);

        tape_len += edge->count;

        node->work.total += child->work.total * edge->count;
        child_work_max = legion_max(child_work_max, child->work.node);

        node->energy.total += child->energy.total * edge->count;
        child_energy_max = legion_max(child_energy_max, child->energy.node);

        for (size_t j = 0; j < child->needs.edges.len; ++j) {
            struct edge *it = child->needs.edges.list + j;
            node_needs_inc(node, it->id, it->count * edge->count);
        }
    }

    if (!node->work.node) {
        assert(child_work_max > 0);
        uint64_t min = child_work_max + 1;
        uint64_t max = legion_max(min * 1.5, min + 1);
        node->work.node = rng_uni(&gen->rng, min, max);
        node->work.node = legion_bound(node->work.node, 1U, UINT8_MAX - tape_len);
    }
    node->work.total += node->work.node;
    node->work.min += node->work.node;

    if (!node->energy.node) {
        assert(child_energy_max > 0);
        uint64_t min = child_energy_max + 1;
        uint64_t max = legion_max(min * 1.5, min + 1);
        node->energy.node = rng_uni(&gen->rng, min, max);
    }
    node->energy.total += node->energy.node * node->work.node;

    node_dump(node, "gen.update");
}

static void gen_node(struct tree *tree, struct node *node)
{
    if (node->done) return;
    info(""); node_dump(node, "gen");

    struct gen gen = {
        .node = node,
        .tree = tree,
        .rng = rng_make(node->id),
        .threshold = 0,
    };

    // For generated nodes, base won't have been initialized so do it here.
    if (node->generated)
        memcpy(&node->base.needs, &node->needs.edges, sizeof(node->base.needs));

    if (!im_type_elem(node->type)) {
        gen_threshold(&gen);
        gen_child_elem(&gen);
        gen_children(&gen);
    }

    gen_host(&gen);
    gen_lab(&gen);

    for (size_t i = 0; i < node->children.edges.len; ++i)
        gen_node(tree, tree_node(tree, node->children.edges.list[i].id));

    gen_update(&gen);
    if (!node->name.len) gen_node_name(&gen);
    node->done = true;
}

static void gen_elem_setup(struct node *node)
{
    node_needs_inc(node, node->id, 1);
}

static void gen_elem_inc(struct tree *tree, struct node *node)
{
    for (size_t i = 0; i < node->children.edges.len; ++i) {
        struct edge *child = node->children.edges.list + i;

        struct node *elem = tree_node(tree, child->id);
        assert(elem && im_type_elem(elem->type));

        for (size_t j = 0; j < elem->needs.edges.len; ++j) {
            struct edge *e = elem->needs.edges.list + j;
            node_needs_inc(node, e->id, e->count * child->count);
        }
    }
}

static void gen_item_inc(struct tree *tree, struct node *node)
{
    struct edges sum = {0};

    for (size_t i = 0; i < node->needs.edges.len; ++i) {
        struct edge *needs = node->needs.edges.list + i;

        struct node *elem = tree_node(tree, needs->id);
        assert(elem && im_type_elem(elem->type));

        for (size_t j = 0; j < elem->needs.edges.len; ++j) {
            struct edge *e = elem->needs.edges.list + j;
            if (e->id != needs->id)
                edges_inc(&sum, e->id, e->count * needs->count);
        }
    }

    for (size_t i = 0; i < sum.len; ++i) {
        const struct edge *e = sum.list + i;
        node_needs_inc(node, e->id, e->count);
    }
}

// If a tape has multiple outputs we divides its needs by the number of outputs
// so that the needs represent the cost for a single element. This avoids having
// to deal with ratios in the rest of the algorithm.
static void gen_out_div(struct node *node)
{
    size_t div = edges_count(&node->out, node->id);
    if (!div) edges_inc(&node->out, node->id, 1);
    if (div <= 1) return;

    for (size_t i = 0; i < node->needs.edges.len; ++i) {
        struct edge *needs = node->needs.edges.list + i;
        needs->count = u64_ceil_div(needs->count, div);
    }
}

static void tech_gen(struct tree *tree)
{
    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (!node || node->type == im_type_sys) continue;

        gen_out_div(node);
        if (im_type_elem(node->type)) gen_elem_setup(node);

        gen_node(tree, node);
    }

    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (!node || node->type == im_type_sys) continue;

        if (im_type_elem(node->type))
            gen_elem_inc(tree, node);
        else gen_item_inc(tree, node);

        node_dump(node, "gen.inc");
    }
}
