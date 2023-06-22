/* tech_parse.c
   Rémi Attab (remi.attab@gmail.com), 21 Feb 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// info
// -----------------------------------------------------------------------------

struct parse_info
{
    uint8_t tier;
    enum im_type type;
    struct symbol syllable, config, list;
    struct { size_t len; char *data; } specs;
};

static void parse_info(struct reader *in, struct parse_info *info)
{
    while (!reader_peek_close(in)) {
        reader_open(in);

        hash_val hash = reader_symbol_hash(in);

        if (hash == symbol_hash_c("tier")) {
            info->tier = reader_word(in);
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("type")) {
            static struct reader_table types[] = {
                { .str = "nil",       .value = im_type_nil },
                { .str = "natural",   .value = im_type_natural },
                { .str = "synth",     .value = im_type_synthetic },
                { .str = "passive",   .value = im_type_passive },
                { .str = "active",    .value = im_type_active },
                { .str = "logistics", .value = im_type_logistics },
                { .str = "sys",       .value = im_type_sys }
            };
            info->type = reader_symbol_table(in, types, array_len(types));
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("syllable")) {
            info->syllable = reader_symbol(in);
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("config")) {
            info->config = reader_symbol(in);
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("list")) {
            info->list = reader_symbol(in);
            reader_close(in);
        }

        else {
            assert(reader_goto_close(in));
            continue;
        }

    }

    reader_close(in);
}

// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

static struct node *parse_tape(
        struct tree *tree, struct reader *in, const struct symbol *item)
{
    struct node *node = NULL;

    while (!reader_peek_close(in)) {
        reader_open(in);

        struct symbol field = reader_symbol(in);
        hash_val hash = symbol_hash(&field);

        if (hash == symbol_hash_c("layer")) {
            if (node) {
                reader_err(in, "duplicate layer field in '%s'", item->c);
                reader_goto_close(in);
                continue;
            }

            uint64_t layer = reader_u64(in);
            node = tree_insert(tree, layer, item);

            reader_close(in);
            continue;
        }

        if (!node) {
            reader_err(in, "missing layer field before '%s' in '%s'", field.c, item->c);
            reader_goto_close(in);
            continue;
        }

        if (hash == symbol_hash_c("host")) {
            node->host.name = reader_symbol(in);
            reader_close(in);
            continue;
        }

        if (hash == symbol_hash_c("work")) {
            node->work.node = reader_u64(in);
            reader_close(in);
            continue;
        }

        if (hash == symbol_hash_c("energy")) {
            node->energy.node = reader_u64(in);
            reader_close(in);
            continue;
        }

        enum { tape_nil, tape_needs, tape_in, tape_out } type = tape_nil;

        if (hash == symbol_hash_c("needs")) type = tape_needs;
        else if (hash == symbol_hash_c("in")) type = tape_in;
        else if (hash == symbol_hash_c("out")) type = tape_out;
        else { reader_goto_close(in); continue; }

        while (!reader_peek_close(in)) {
            reader_open(in);

            struct symbol sym = reader_symbol(in);
            struct node *child = tree_symbol(tree, &sym);
            if (!child) {
                reader_err(in, "unknown tape entry '%s' in '%s'", sym.c, item->c);
                reader_goto_close(in);
                continue;
            }

            uint32_t count = reader_u64(in);
            switch (type) {
            case tape_needs: { node_needs_inc(node, child->id, count); break; }
            case tape_in: { node_child_inc(node, child->id, count); break; }
            case tape_out: { node->out = edges_inc(node->out, child->id, count); break; }
            default: { assert(false); }
            }

            reader_close(in);
        }

        reader_close(in);
    }
    reader_close(in);

    node->base.in = edges_copy(node->children.edges);
    node->base.needs = edges_copy(node->needs.edges);

    return node;
}

// -----------------------------------------------------------------------------
// tech
// -----------------------------------------------------------------------------

static void tech_parse(struct tree *tree, const char *path)
{
    struct config config = {0};
    struct reader *in = config_read(&config, path);

    while (!reader_peek_eof(in)) {
        reader_open(in);

        struct parse_info info = {0};
        struct symbol item = reader_symbol(in);

        while (!reader_peek_close(in)) {
            reader_open(in);

            struct symbol field = reader_symbol(in);
            hash_val hash = symbol_hash(&field);

            if (hash == symbol_hash_c("info")) {
                parse_info(in, &info);
                continue;
            }

            else if (hash == symbol_hash_c("specs")) {
                enum { cap = 256 };
                info.specs.data = calloc(cap, sizeof(char));
                info.specs.len = reader_until_close(in, info.specs.data, cap);
                assert(info.specs.len < cap - 1);
                continue;
            }

            else if (hash == symbol_hash_c("tape")) {
                if (info.type == im_type_nil) {
                    reader_err(in, "missing 'info.type' field before 'tape' field for '%s'",
                            item.c);
                    assert(reader_goto_close(in));
                    continue;
                }

                struct node *node = parse_tape(tree, in, &item);
                node->name = item;
                node->type = info.type;
                node->tier = info.tier;
                node->syllable = info.syllable;
                node->config = info.config;
                node->list = info.list;
                node->specs.len = info.specs.len;
                node->specs.data = info.specs.data;
            }

            else assert(reader_goto_close(in));
        }

        reader_close(in);
    }

    config_close(&config);
}
