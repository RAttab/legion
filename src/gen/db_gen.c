/* db_items.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// items
// -----------------------------------------------------------------------------

static void db_gen_items(struct db_state *state)
{
    int cmp(const void *lhs_, const void *rhs_) {
        const struct db_info *lhs = lhs_;
        const struct db_info *rhs = rhs_;

        if (lhs->type != rhs->type) return lhs->type - rhs->type;
        if (lhs->order != rhs->order) return lhs->order - rhs->order;
        if (!im_type_elem(lhs->type) && lhs->layer != rhs->layer)
            return lhs->layer - rhs->layer;
        return symbol_cmp(&lhs->name, &rhs->name);
    }
    vec_info_sort_fn(state->info, cmp);

    void write_bounds_end(enum im_type type, int64_t atom) {
        struct symbol sym_type = make_symbol(im_type_str(type));
        db_file_writef(&state->files.im_enum,
                "  items_%s_last = 0x%02lx,\n"
                "  items_%s_len = items_%s_last - items_%s_first,\n",
                sym_type.c, atom,
                sym_type.c, sym_type.c, sym_type.c);
    }

    enum im_type type = im_type_nil;
    for (size_t i = 0; i < state->info->len; ++i) {
        struct db_info *info = state->info->vals + i;

        info->atom = i + 1;
        struct symbol atom = symbol_concat("item-", info->name.c);
        bool ok = state_atoms_set(state, &atom, info->atom);
        if (!ok) errf("duplicate item: %s", atom.c);

        if (type != info->type) {
            if (type) write_bounds_end(type, info->atom);
            type = info->type;
            struct symbol sym_type = make_symbol(im_type_str(type));
            db_file_write_sep(&state->files.im_enum, sym_type.c);
            db_file_write_sep(&state->files.im_register, sym_type.c);
            db_file_writef(&state->files.im_enum,
                    "  items_%s_first = 0x%02lx,\n", sym_type.c, info->atom);
        }

        struct symbol sym_enum = symbol_to_enum(info->name);

        db_file_writef(&state->files.im_enum,
                "  item_%-30s = 0x%02lx,\n", sym_enum.c, info->atom);

        if (info->type != im_type_active) {
            db_file_writef(&state->files.im_register,
                    "im_register(item_%s, \"%s\", %u, \"item-%s\"),\n",
                    sym_enum.c, info->name.c, info->name.len, info->name.c);
        }
        else if (info->config.len) {
            db_file_writef(&state->files.im_register,
                    "im_register_cfg(item_%s, \"%s\", %u, \"item-%s\", im_%s_config),\n",
                    sym_enum.c, info->name.c, info->name.len, info->name.c, info->config.c);
        }
        else {
            db_file_writef(&state->files.im_register,
                    "im_register_cfg(item_%s, \"%s\", %u, \"item-%s\", im_%s_config),\n",
                    sym_enum.c, info->name.c, info->name.len, info->name.c, sym_enum.c);
            db_file_writef(&state->files.im_includes,
                    "#include \"items/%s/%s.h\"\n", sym_enum.c, sym_enum.c);
        }

        if (info->list == list_control)
            db_file_writef(&state->files.im_control, "item_%s,\n", sym_enum.c);
        if (info->list == list_factory)
            db_file_writef(&state->files.im_factory, "item_%s,\n", sym_enum.c);

        if (i == state->info->len - 1)
            write_bounds_end(type, info->atom + 1);
    }

    db_file_writef(&state->files.im_enum,
            "\n  items_max = 0x%02x,\n", state->info->len + 1);
}


// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

static void db_gen_specs(
        struct db_state *state, struct reader *in, const struct symbol *item)
{
    struct symbol item_enum = symbol_to_enum(*item);

    uint8_t seq = 0;
    while (!reader_peek_close(in)) {
        reader_open(in);

        struct symbol spec = reader_symbol(in);
        struct symbol spec_enum = symbol_to_enum(spec);

        hash_val hash = symbol_hash(&spec);
        if (    hash == symbol_hash_c("lab-bits") ||
                hash == symbol_hash_c("lab-work") ||
                hash == symbol_hash_c("lab-energy"))
        {
            db_file_writef(&state->files.specs_enum,
                    "\nspec_%s_%s = make_spec(item_%s, spec_%s),",
                    item_enum.c, spec_enum.c, item_enum.c, spec_enum.c);
        }
        else {
            db_file_writef(&state->files.specs_enum,
                    "\nspec_%s_%s = make_spec(item_%s, 0x%x),",
                    item_enum.c, spec_enum.c, item_enum.c, seq++);
        }


        struct symbol type = reader_symbol(in);
        hash_val type_hash = symbol_hash(&type);

        if (type_hash == symbol_hash_c("fn")) {
            db_file_writef(&state->files.specs_register,
                    "spec_register_fn("
                    "spec_%s_%s, "
                    "\"spec-%s-%s\", "
                    "spec_%s_%s_fn);\n",
                    item_enum.c, spec_enum.c,
                    item->c, spec.c,
                    item_enum.c, spec_enum.c);
            reader_close(in);
            continue;
        }

        db_file_writef(&state->files.specs_register,
                "spec_register_var("
                "spec_%s_%s, "
                "\"spec-%s-%s\", "
                "im_%s_%s);\n",
                item_enum.c, spec_enum.c,
                item->c, spec.c,
                item_enum.c, spec_enum.c);


        bool is_enum = false;
        if (type_hash == symbol_hash_c("word"))
            type = make_symbol("vm_word");

        else if (type_hash == symbol_hash_c("item"))
            type = make_symbol("enum item");

        else if (type_hash == symbol_hash_c("work"))
            type = make_symbol("im_work");

        else if (type_hash == symbol_hash_c("u8"))
            type = make_symbol("uint8_t");

        else if (type_hash == symbol_hash_c("u16"))
            type = make_symbol("uint16_t");

        else if (type_hash == symbol_hash_c("u32"))
            type = make_symbol("uint32_t");

        else if (type_hash == symbol_hash_c("energy"))
            type = make_symbol("im_energy");

        else if (type_hash == symbol_hash_c("enum"))
            is_enum = true;

        else {
            reader_err(in, "unknown type '%s'", type.c);
            reader_goto_close(in);
            continue;
        }

        if (is_enum) db_file_writef(&state->files.specs_value,
                "enum { im_%s_%s = ", item_enum.c, spec_enum.c);
        else db_file_writef(&state->files.specs_value,
                "static const %s im_%s_%s = ", type.c, item_enum.c, spec_enum.c);

        enum token_type token = reader_peek(in);
        switch (token) {
        case token_number: {
            db_file_writef(&state->files.specs_value, "0x%lx", reader_word(in));
            break;
        }
        case token_atom: {
            struct symbol atom = symbol_to_enum(reader_atom_symbol(in));
            db_file_writef(&state->files.specs_value, "%s", atom.c);
            break;
        }
        default: {
            reader_err(in, "unexpected token type: %s", token_type_str(token));
            reader_goto_close(in);
            continue;
        }
        }

        db_file_write(&state->files.specs_value, is_enum ? " };\n" : ";\n");

        reader_close(in);
    }

    reader_close(in);

    db_file_write(&state->files.specs_enum, "\n");
    db_file_write(&state->files.specs_value, "\n");
    db_file_write(&state->files.specs_register, "\n");
}


// -----------------------------------------------------------------------------
// tape-info
// -----------------------------------------------------------------------------

static void db_gen_tape_info(
        struct db_state *state, struct reader *in, const struct symbol *item)
{
    uint8_t rank = 0;
    uint16_t elems[12] = {0};

    static struct bits tech = {0};
    bits_clear(&tech);

    while (!reader_peek_close(in)) {
        reader_open(in);

        struct symbol key = reader_symbol(in);
        hash_val hash = symbol_hash(&key);

        if (hash == symbol_hash_c("rank")) {
            rank = reader_word(in);
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("elems")) {
            while (!reader_peek_close(in)) {
                reader_open(in);
                struct symbol elem = reader_symbol(in);
                uint64_t count = reader_word(in);

                uint64_t atom = state_atoms_value(state, &elem);
                if (atom < array_len(elems)) elems[atom] = count;

                reader_close(in);
            }
            reader_close(in);
        }

        else if (hash == symbol_hash_c("tech")) {
            while (!reader_peek_close(in)) {
                reader_open(in);
                struct symbol item = reader_symbol(in);

                uint64_t atom = state_atoms_value(state, &item);
                assert(atom > 0 && atom < UINT8_MAX);
                bits_put(&tech, atom);

                reader_close(in);
            }
            reader_close(in);
        }
    }

    db_file_writef(&state->files.tapes_info,
            "\ntape_info_register_begin(item_%s) { .rank = %u };\n",
            item->c, rank);

    for (size_t value = bits_next(&tech, 0);
         value < tech.len; value = bits_next(&tech, value + 1))
    {
        db_file_writef(&state->files.tapes_info,
                "  tape_info_register_tech(%s);\n",
                symbol_to_enum(state_atoms_name(state, value)).c);
    }

    for (size_t i = 1; i < array_len(elems); ++i) {
        if (!elems[i]) continue;
        db_file_writef(&state->files.tapes_info,
                "  tape_info_register_elems(%s, %u);\n",
                symbol_to_enum(state_atoms_name(state, i)).c, elems[i]);
    }

    db_file_write(&state->files.tapes_info, "tape_info_register_end()\n");
}


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

static void db_gen_tape(
        struct db_state *state, struct reader *in, const struct symbol *item)
{
    uint8_t work = 0;
    uint32_t energy = 0;
    struct symbol host = {0};

    size_t input = 0, output = 0;
    struct symbol tape[256] = {0};

    struct symbol item_enum = symbol_to_enum(*item);

    while (!reader_peek_close(in)) {
        reader_open(in);

        struct symbol key = reader_symbol(in);
        hash_val hash = symbol_hash(&key);

        if (hash == symbol_hash_c("work")) {
            int64_t value = reader_word(in);
            if (value < 0 || value > UINT8_MAX)
                reader_err(in, "invalid work value '%lx'", value);
            work = value;
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("energy")) {
            int64_t value = reader_word(in);
            if (value < 0 || value > UINT32_MAX)
                reader_err(in, "invalid energy value '%lx'", value);
            energy = value;
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("host")) {
            struct symbol value = reader_symbol(in);
            if (!state_atoms_value(state, &value))
                reader_err(in, "unknown host atom '%s'", value.c);
            host = symbol_to_enum(value);
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("info")) {
            db_gen_tape_info(state, in, &item_enum);
            reader_close(in);
            continue;
        }

        bool is_input = hash == symbol_hash_c("in");
        bool is_output = hash == symbol_hash_c("out");
        if (!is_input && !is_output) {
            reader_err(in, "unknown field '%s'", key.c);
            reader_goto_close(in);
            continue;
        }

        while (!reader_peek_close(in)) {
            int64_t count = 1;
            struct symbol item = {0};

            if (reader_peek(in) == token_symbol)
                item = reader_symbol(in);
            else {
                reader_open(in);
                item = reader_symbol(in);
                count = reader_word(in);
                reader_close(in);
            }

            if (!state_atoms_value(state, &item))
                reader_err(in, "unknown atom '%s'", item.c);

            if (count < 1 || count > UINT8_MAX) {
                reader_err(in, "invalid count '%lx'", count);
                count = 0;
            }

            if (count + input + output > UINT8_MAX) {
                reader_err(in, "tape overflow: %zu + %zu + %zu", count, input, output);
                count = 0;
            }

            item = symbol_to_enum(item);
            for (size_t i = 0; i < (size_t) count; ++i) {
                if (is_input) tape[input++] = item;
                if (is_output) tape[input + output++] = item;
            }
        }

        reader_close(in);
    }
    reader_close(in);

    db_file_writef(&state->files.tapes,
            "\ntape_register_begin(item_%s, %zu) {\n"
            "    .id = item_%s,\n"
            "    .host = %s,\n"
            "    .work = %u,\n"
            "    .energy = %u,\n"
            "    .inputs = %zu,\n"
            "    .outputs = %zu,\n"
            "  };\n",
            item_enum.c, input + output,
            item_enum.c, host.c, work, energy, input, output);

    for (size_t i = 0; i < input + output; ++i) {
        db_file_writef(&state->files.tapes,
                "  tape_register_ix(%3zu, %s);\n", i, tape[i].c);
    }

    db_file_write(&state->files.tapes, "tape_register_end()\n");
}


// -----------------------------------------------------------------------------
// specs-tape
// -----------------------------------------------------------------------------

static void db_gen_specs_tapes(struct db_state *state, const char *file)
{
    struct config config = {0};
    struct reader *in = config_read(&config, file);

    while (!reader_peek_eof(in)) {
        reader_open(in);
        struct symbol item = reader_symbol(in);

        while (!reader_peek_close(in)) {
            reader_open(in);

            struct symbol section = reader_symbol(in);
            hash_val hash = symbol_hash(&section);

            if (hash == symbol_hash_c("info"))
                reader_goto_close(in);

            else if (hash == symbol_hash_c("specs"))
                db_gen_specs(state, in, &item);

            else if (hash == symbol_hash_c("tape"))
                db_gen_tape(state, in, &item);

            else if (hash == symbol_hash_c("dbg"))
                reader_goto_close(in);

            else {
                reader_err(in, "unknown section: %s", section.c);
                reader_goto_close(in);
            }

        }

        reader_close(in);
    }

    config_close(&config);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void db_gen_io(struct db_state *state, const char *file)
{
    struct config config = {0};
    struct reader *in = config_read(&config, file);

    while (!reader_peek_eof(in)) {
        reader_open(in);
        struct symbol type = reader_symbol(in);
        hash_val hash = symbol_hash(&type);

        if (hash == symbol_hash_c("io")) {
            for (size_t i = 0; !reader_peek_close(in); i++) {
                struct symbol io = reader_symbol(in);
                struct symbol io_enum = symbol_to_enum(io);

                db_file_writef(&state->files.io_enum,
                        "%-20s = io_min + 0x%02lx,\n", io_enum.c, i);
                db_file_writef(&state->files.io_register,
                        "io_register(%s, \"%s\", %u),\n",
                        io_enum.c, io.c, io.len);
            }

            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("ioe")) {
            for (size_t i = 0; !reader_peek_close(in); i++) {
                struct symbol ioe = reader_symbol(in);
                struct symbol ioe_enum = symbol_to_enum(ioe);

                db_file_writef(&state->files.ioe_enum,
                        "%-20s = ioe_min + 0x%02lx,\n", ioe_enum.c, i);
                db_file_writef(&state->files.io_register,
                        "ioe_register(%s, \"%s\", %u),\n",
                        ioe_enum.c, ioe.c, ioe.len);
            }

            reader_close(in);
            continue;
        }

        else {
            reader_err(in, "unknown type io '%s'", type.c);
            reader_goto_close(in);
            continue;
        }
    }

    config_close(&config);
}
