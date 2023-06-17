/* tech_check.c
   Rémi Attab (remi.attab@gmail.com), 09 Apr 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// check inputs
// -----------------------------------------------------------------------------

static void check_basics(struct node *node)
{
    if (!node->work.node  && im_type_elem(node->type))
        errf("[%02x:%s] missing work", node->id, node->name.c);

    if (!node->energy.node && im_type_elem(node->type))
        errf("[%02x:%s] missing energy:node", node->id, node->name.c);
}

static void check_inputs_sum_needs(
        struct tree *tree,
        struct node *node,
        struct edges *sum,
        size_t count)
{
    if (node->base.needs.len) {
        for (size_t i = 0; i < node->base.needs.len; ++i) {
            const struct edge *need = node->base.needs.list + i;
            edges_inc(sum, need->id, need->count * count);
        }
        return;
    }

    if (node->base.in.len) {
        for (size_t i = 0; i < node->base.in.len; ++i) {
            const struct edge *in = node->base.in.list + i;
            check_inputs_sum_needs(
                    tree, tree_node(tree, in->id),
                    sum, in->count * count);
        }
        return;
    }

    edges_inc(sum, node->id, count);
}

static void check_inputs_sum_elems(
        struct tree *tree,
        struct node *node,
        struct edges *sum,
        uint32_t count)
{
    size_t div = legion_min(1U, edges_count(&node->out, node->id));
    for (size_t i = 0; i < node->base.in.len; ++i) {
        struct edge *elem = node->base.in.list + i;
        edges_inc(sum, elem->id, elem->count * count);
        check_inputs_sum_elems(
                tree, tree_node(tree, elem->id),
                sum, (elem->count / div) * count);
    }
}

static void check_inputs_needs(struct tree *tree, struct node *node)
{
    if (!node->base.needs.len) return;

    struct edges ins = {0};
    edges_init(&ins);

    for (size_t i = 0; i < node->base.in.len; ++i) {
        struct edge *in = node->base.in.list + i;
        check_inputs_sum_needs(
                tree, tree_node(tree, in->id),
                &ins, in->count);
    }

    struct edges *needs = &node->needs.edges;

    for (size_t i = 0; i < ins.len; ++i) {
        struct edge *exp = ins.list + i;
        uint32_t val = edges_count(needs, exp->id);
        if (val >= exp->count) continue;

        errf("[%02x:%s] inputs.ins: field=%02x:%s, val=%u, needs=%u",
                node->id, node->name.c,
                exp->id, tree_name(tree, exp->id).c,
                val, exp->count);
    }

    for (size_t i = 0; i < ins.len; ++i)
        node_needs_dec(node, ins.list[i].id, ins.list[i].count);

    struct edges elems = {0};
    edges_init(&elems);

    for (size_t i = 0; i < needs->len; ++i) {
        struct edge *need = needs->list + i;
        check_inputs_sum_elems(
                tree, tree_node(tree, need->id),
                &elems, need->count);
    }

    for (size_t i = 0; i < elems.len; ++i) {
        struct edge *exp = elems.list + i;

        uint32_t val = edges_count(needs, exp->id);
        if (val >= exp->count) continue;

        uint32_t in = edges_count(&ins, exp->id);
        uint32_t base = edges_count(&node->base.needs, exp->id);

        errf("[%02x:%s] inputs.elems: field=%02x:%s, val=%u, exp=%u | %u >= %u { ins=%u + elems=%u }",
                node->id, node->name.c,
                exp->id, tree_name(tree, exp->id).c,
                val, exp->count, base, in + exp->count, in, exp->count);
    }

    for (size_t i = 0; i < elems.len; ++i)
        node_needs_dec(node, elems.list[i].id, elems.list[i].count);
}

static void tech_check_inputs(struct tree *tree)
{
    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (!node || node->type == im_type_sys) continue;

        check_basics(node);
        check_inputs_needs(tree, node);
    }
}


// -----------------------------------------------------------------------------
// check outputs
// -----------------------------------------------------------------------------

static void check_delta(
        const char *field, struct node *node, uint32_t value, uint32_t exp)
{
    if (!exp) {
        errf("[%02x:%s] field=%s, exp=%u, has=%u",
                node->id, node->name.c, field, exp, value);
        return;
    }

    const uint32_t delta = legion_max((exp * check_mult) / check_div, 1U);
    const uint32_t min = exp - legion_min(exp, delta);
    const uint32_t max = exp + delta;

    if (value >= min && value <= max) return;

    errf("[%02x:%s] field=%s, exp={%u +/- %u}, has={%u <= %u <= %u}",
            node->id, node->name.c, field,
            exp, delta, min, value, max);
}

static void check_delta_id(
        const char *field,
        struct tree *tree, struct node *node,
        node_id id, uint32_t value, uint32_t exp)
{
    char buf[64] = {0};
    snprintf(buf, sizeof(buf),
            "%s:%02x:%s", field, id, tree_name(tree, id).c);

    check_delta(buf, node, value, exp);
}

static void check_tape(struct node *node)
{
    size_t ins = 0;
    size_t work = node->work.node;
    size_t outs = legion_max(node->out.len, 1U);
    for (size_t i = 0; i < node->children.edges.len; ++i)
        ins += node->children.edges.list[i].count;

    size_t total = ins + work + outs;
    if (total >= UINT8_MAX) {
        errf("[%02x:%s] tape length: ins=%zu + work=%zu + outs=%zu = %zu",
                node->id, node->name.c, ins, work, outs, total);
    }
}

static void check_needs(struct tree *tree, struct node *node)
{
    struct bits base = {0};
    for (size_t i = 0; i < node->base.needs.len; ++i)
        bits_put(&base, node->base.needs.list[i].id);

    struct bits bits = {0};

    bits_copy(&bits, &node->needs.set);
    bits_minus(&bits, &base);
    for (node_id id = bits_next(&bits, 0);
         id < bits.len; id = bits_next(&bits, id + 1))
    {
        errf("[%02x:%s] missing needs: id=%02x:%s",
                node->id, node->name.c, id, tree_name(tree, id).c);
    }

    bits_copy(&bits, &base);
    bits_minus(&bits, &node->needs.set);
    for (node_id id = bits_next(&bits, 0);
         id < bits.len; id = bits_next(&bits, id + 1))
    {
        errf("[%02x:%s] extra needs: id=%02x:%s",
                node->id, node->name.c, id, tree_name(tree, id).c);
    }

    bits_copy(&bits, &base);
    bits_intersect(&bits, &node->needs.set);
    for (node_id id = bits_next(&bits, 0);
         id < bits.len; id = bits_next(&bits, id + 1))
    {
        const struct edge *base = edges_find(&node->base.needs, id);
        const struct edge *need = edges_find(&node->needs.edges, id);
        check_delta_id("need", tree, node, id, need->count, base->count);
    }

    bits_free(&bits);
    bits_free(&base);
}

static void check_children(struct tree *tree, struct node *node)
{
    for (size_t i = 0; i < node->children.edges.len; ++i) {
        const struct edge *edge = node->children.edges.list + i;

        if (node->children.edges.len == 1 && edge->count == 1) {
            errf("[%02x:%s] singleton: id=%02x:%s",
                    node->id, node->name.c,
                    edge->id, tree_name(tree, edge->id).c);
        }
    }
}

static void check_deps(struct tree *tree, struct node *node, struct bits *set)
{
    size_t count = 0;

    char buffer[256] = {0};
    char *it = buffer;
    char *const end = it + sizeof(buffer);

    void check_node(struct node *node) {
        bits_put(set, node->id);
        for (size_t i = 0; i < node->children.edges.len; ++i) {
            struct edge *edge = node->children.edges.list + i;
            if (bits_has(set, edge->id)) continue;

            struct node *child = tree_node(tree, edge->id);
            if (im_type_elem(child->type)) continue;

            count++;
            it += snprintf(it, end - it, " %02x:%s", child->id, child->name.c);
            check_node(child);
        }
    }

    check_node(node);
    if (!count) return;

    infof("[%02x:%s] new-deps %zu:[%s ]", node->id, node->name.c, count, buffer);
}

static void check_host(struct tree *tree, struct node *node)
{
    if (!node->host.name.len) return;

    struct node *host = tree_symbol(tree, &node->host.name);
    if (host) return;

    errf("[%02x:%s] unknown host: %s", node->id, node->name.c, node->host.name.c);
}

static void tech_check_outputs(struct tree *tree)
{
    struct bits deps = {0};

    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (!node || node->type == im_type_sys || im_type_elem(node->type))
            continue;

        check_tape(node);
        check_children(tree, node);
        if (!node->generated && node->base.needs.len)
            check_needs(tree, node);

        if (!node->generated)
            check_deps(tree, node, &deps);
    }

    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (node) check_host(tree, node);
    }

    bits_free(&deps);
}
