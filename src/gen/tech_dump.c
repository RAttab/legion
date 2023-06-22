/* tech_dump.c
   Rémi Attab (remi.attab@gmail.com), 23 Feb 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// lisp
// -----------------------------------------------------------------------------

static void dump_tape(
        struct mfile_writer *out, struct tree *tree, struct node *node)
{
    if (!edges_len(node->children.edges)) return;

    struct rng rng = rng_make(node->id);
    struct edges *ins = edges_copy(node->children.edges);

    static struct edge tape[256] = {0};
    size_t front = 0, back = array_len(tape);

    while (edges_len(ins)) {
        size_t i = rng_uni(&rng, 0, edges_len(ins));
        struct edge in = ins->vals[i];
        assert(in.count > 0);

        if (edges_len(ins) == 1) {
            if (front && tape[front-1].id == in.id)
                tape[front-1].count += in.count;
            else if (back < array_len(tape) && tape[back].id == in.id)
                tape[front].count += in.count;
            else tape[front++] = in;
            break;
        }

        enum { op_nil = 0, op_front, op_back, op_both } op = op_nil;
        uint32_t count = in.count;

        if (in.count > 1 && rng_prob(&rng, 0.5)) {
            op = op_both;
            count = rng_uni(&rng, 0, count / 2) + 1;
        }
        else {
            op = rng_prob(&rng, 0.5) ? op_front : op_back;
            count = rng_uni(&rng, 0, count) + 1;
        }
        struct edge edge = { .id = in.id, .count = count };

        if (op == op_front || op == op_both) {
            if (front && tape[front-1].id == edge.id)
                tape[front-1].count += edge.count;
            else tape[front++] = edge;
            edges_dec(ins, edge.id, edge.count);
        }

        if (op == op_back || op == op_both) {
            if (back < array_len(tape) && tape[back].id == edge.id)
                tape[back].count += edge.count;
            else tape[--back] = edge;
            edges_dec(ins, edge.id, edge.count);
        }

        assert(front < back);
    }

    edges_free(legion_xchg(&ins, NULL));

    mfile_write(out, "    (in ");
    for (size_t i = 0; true; i++) {
        if (i == front) i = back;
        if (i == array_len(tape)) break;

        struct edge *edge = tape + i;
        struct node *child = tree_node(tree, edge->id);
        mfile_writef(out, "%s(item-%s %u)",
                i ? "\n        " : "", child->name.c, edge->count);
    }
    mfile_write(out, ")\n");

}

static void dump_lisp_node(
        struct mfile_writer *out, struct tree *tree, struct node *node)
{
    mfile_writef(out, "(%s\n", node->name.c);

    { // info
        mfile_writef(out, "  (info (type %s)", im_type_str(node->type));
        if (node->config.len)
            mfile_writef(out, " (config %s)", node->config.c);
        if (node->list.len)
            mfile_writef(out, " (list %s)", node->list.c);

        if (node->type != im_type_sys)
            mfile_write(out, ")\n");
        else {
            mfile_write(out, "))\n\n");
            return;
        }
    }

    { // specs
        mfile_write(out, "  (specs");

        mfile_writef(out,
                "%s(lab-bits u8 %u) (lab-work work %u) (lab-energy energy %u)",
                node->specs.len ? "\n    " : " ",
                node->lab.bits, node->lab.work, node->lab.energy);

        if (node->specs.len)
            mfile_writef(out, "\n    %*s",
                    (unsigned) node->specs.len, node->specs.data);

        mfile_write(out, ")\n");
    }

    { // tape
        mfile_write(out, "  (tape");

        struct symbol *host = &node->host.name;
        struct node *n = tree_node(tree, node->host.id);
        if (n) host = &n->name;

        mfile_writef(out, " (work %u) (energy %u) (host item-%s)\n",
                node->work.node, node->energy.node, host->c);
        dump_tape(out, tree, node);
        mfile_writef(out, "    (out (item-%s 1)))\n", node->name.c);
    }

    { // dbg
        mfile_write(out, "  (dbg\n");

        mfile_writef(out, "    (info (id %x) (layer %u))\n",
                node->id, node_id_layer(node->id));

        mfile_writef(out, "    (work (min %u) (total %u))\n",
                node->work.min, node->work.total);

        mfile_writef(out, "    (energy %u)\n", node->energy.total);

        mfile_writef(out, "    (children %zu", edges_len(node->children.edges));
        for (size_t i = 0; i < edges_len(node->children.edges); ++i) {
            struct edge *edge = node->children.edges->vals + i;
            struct node *child = tree_node(tree, edge->id);
            mfile_writef(out, "\n      (%02x %s %u)",
                    child->id, child->name.c, edge->count);
        }
        mfile_write(out, ")\n");

        mfile_writef(out, "    (needs %zu", edges_len(node->needs.edges));
        for (size_t i = 0; i < edges_len(node->needs.edges); ++i) {
            struct edge *edge = node->needs.edges->vals + i;
            struct node *child = tree_node(tree, edge->id);
            mfile_writef(out, "\n      (%02x %s %u)",
                    child->id, child->name.c, edge->count);
        }
        mfile_write(out, ")");

        mfile_write(out, ")");
    }

    mfile_write(out, ")\n\n");
}


// -----------------------------------------------------------------------------
// dot
// -----------------------------------------------------------------------------

static void dump_dot_node(
        struct mfile_writer *out, struct node *node)
{
    const char *color = "gray";
    switch (node->type) {
    case im_type_natural:   { color = "blue"; break; }
    case im_type_synthetic: { color = "purple"; break; }
    case im_type_active:    { color = "red"; break; }
    case im_type_logistics: { color = "orange"; break; }
    case im_type_passive:   { color = "green"; break; }
    default: { return; }
    }

    mfile_writef(out,
            "subgraph { node [color=%s; label=\"%02x:%s\"]; \"%02x\" }\n",
            color, node->id, node->name.c, node->id);

    for (size_t i = 0; i < edges_len(node->children.edges); ++i) {
        struct edge *child = node->children.edges->vals + i;
        mfile_writef(out, "\"%02x\" -> \"%02x\" [headlabel=\"%u\"; arrowsize=0.5]\n",
                child->id, node->id, child->count);
    }

    mfile_write(out, "\n");
}

static void dump_dot_suffix(struct mfile_writer *out)
{
    mfile_write(out, "}\n");
}


// -----------------------------------------------------------------------------
// dump
// -----------------------------------------------------------------------------

static void tech_dump(struct tree *tree, const char *src, const char *output)
{
    char path[PATH_MAX] = {0};

    struct mfile_writer lisp = {0};
    snprintf(path, sizeof(path), "%s/tech.lisp", src);
    mfile_writer_open(&lisp, path, 1048576);

    struct mfile_writer dot = {0};
    snprintf(path, sizeof(path), "%s/tech.dot", output);
    mfile_writer_open(&dot, path, 1048576);
    mfile_write(&dot, "strict digraph {\n\n");

    for (node_id id = 0; id < node_id_max; ++id) {
        struct node *node = tree_node(tree, id);
        if (!node) continue;

        dump_lisp_node(&lisp, tree, node);
        dump_dot_node(&dot, node);
    }

    dump_dot_suffix(&dot);
    mfile_writer_close(&dot);
    mfile_writer_close(&lisp);
}
