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

static struct edges *check_inputs_sum_needs(
        struct tree *tree,
        struct node *node,
        struct edges *sum,
        size_t count)
{
    if (edges_len(node->base.needs)) {
        for (size_t i = 0; i < edges_len(node->base.needs); ++i) {
            const struct edge *need = node->base.needs->vals + i;
            sum = edges_inc(sum, need->id, need->count * count);
        }
        return sum;
    }

    if (edges_len(node->base.in)) {
        for (size_t i = 0; i < edges_len(node->base.in); ++i) {
            const struct edge *in = node->base.in->vals + i;
            sum = check_inputs_sum_needs(
                    tree, tree_node(tree, in->id),
                    sum, in->count * count);
        }
        return sum;
    }

    return edges_inc(sum, node->id, count);
}

static struct edges *check_inputs_sum_elems(
        struct tree *tree,
        struct node *node,
        struct edges *sum,
        uint32_t count)
{
    size_t div = legion_min(1U, edges_count(node->out, node->id));
    for (size_t i = 0; i < edges_len(node->base.in); ++i) {
        struct edge *elem = node->base.in->vals + i;
        sum = edges_inc(sum, elem->id, elem->count * count);
        sum = check_inputs_sum_elems(
                tree, tree_node(tree, elem->id),
                sum, (elem->count / div) * count);
    }
    return sum;
}

static void check_inputs_needs(struct tree *tree, struct node *node)
{
    if (!edges_len(node->base.needs)) return;

    struct edges *ins = NULL;
    for (size_t i = 0; i < edges_len(node->base.in); ++i) {
        struct edge *in = node->base.in->vals + i;
        ins = check_inputs_sum_needs(
                tree, tree_node(tree, in->id),
                ins, in->count);
    }

    struct edges *needs = node->needs.edges;
    for (size_t i = 0; i < edges_len(ins); ++i) {
        struct edge *exp = ins->vals + i;
        uint32_t val = edges_count(needs, exp->id);
        if (val >= exp->count) continue;

        errf("[%02x:%s] inputs.ins: field=%02x:%s, val=%u, needs=%u",
                node->id, node->name.c,
                exp->id, tree_name(tree, exp->id).c,
                val, exp->count);
    }

    for (size_t i = 0; i < edges_len(ins); ++i)
        node_needs_dec(node, ins->vals[i].id, ins->vals[i].count);

    struct edges *elems = NULL;
    for (size_t i = 0; i < edges_len(needs); ++i) {
        struct edge *need = needs->vals + i;
        elems = check_inputs_sum_elems(
                tree, tree_node(tree, need->id),
                elems, need->count);
    }

    for (size_t i = 0; i < edges_len(elems); ++i) {
        struct edge *exp = elems->vals + i;

        uint32_t val = edges_count(needs, exp->id);
        if (val >= exp->count) continue;

        uint32_t in = edges_count(ins, exp->id);
        uint32_t base = edges_count(node->base.needs, exp->id);

        errf("[%02x:%s] inputs.elems: field=%02x:%s, val=%u, exp=%u | %u >= %u { ins=%u + elems=%u }",
                node->id, node->name.c,
                exp->id, tree_name(tree, exp->id).c,
                val, exp->count, base, in + exp->count, in, exp->count);
    }

    for (size_t i = 0; i < edges_len(elems); ++i)
        node_needs_dec(node, elems->vals[i].id, elems->vals[i].count);

    edges_free(ins);
    edges_free(elems);
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
    size_t outs = legion_max(edges_len(node->out), 1U);
    for (size_t i = 0; i < edges_len(node->children.edges); ++i)
        ins += node->children.edges->vals[i].count;

    size_t total = ins + work + outs;
    if (total >= UINT8_MAX) {
        errf("[%02x:%s] tape length: ins=%zu + work=%zu + outs=%zu = %zu",
                node->id, node->name.c, ins, work, outs, total);
    }
}

static void check_needs(struct tree *tree, struct node *node)
{
    struct bits base = {0};
    for (size_t i = 0; i < edges_len(node->base.needs); ++i)
        bits_put(&base, node->base.needs->vals[i].id);

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
        const struct edge *base = edges_find(node->base.needs, id);
        const struct edge *need = edges_find(node->needs.edges, id);
        check_delta_id("need", tree, node, id, need->count, base->count);
    }

    bits_free(&bits);
    bits_free(&base);
}

static void check_children(struct tree *tree, struct node *node)
{
    for (size_t i = 0; i < edges_len(node->children.edges); ++i) {
        const struct edge *edge = node->children.edges->vals + i;

        if (edges_len(node->children.edges) == 1 && edge->count == 1) {
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
        for (size_t i = 0; i < edges_len(node->children.edges); ++i) {
            struct edge *edge = node->children.edges->vals + i;
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
    size_t generated = 0;

    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (!node || node->type == im_type_sys || im_type_elem(node->type))
            continue;

        check_tape(node);
        check_children(tree, node);
        if (!node->generated && edges_len(node->base.needs))
            check_needs(tree, node);

        if (node->generated) generated++;
        else check_deps(tree, node, &deps);
    }

    infof("generated: %zu", generated);

    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (node) check_host(tree, node);
    }

    bits_free(&deps);
}
