/* db_stars.c
   Rémi Attab (remi.attab@gmail.com), 19 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// names
// -----------------------------------------------------------------------------

struct db_names
{
    size_t len;
    struct symbol name;
    struct symbol list[256];
};

static void db_names_read(struct reader *in, struct db_names *names)
{
    reader_open(in);
    names->len = 0;
    names->name = reader_symbol(in);
    while (!reader_peek_close(in)) {
        names->list[names->len] = reader_symbol(in);
        names->len++;
    };
    reader_close(in);

    int cmp(const void *lhs, const void *rhs) { return symbol_cmp(lhs, rhs); }
    qsort(names->list, names->len, sizeof(names->list[0]), &cmp);
}

static void db_gen_prefix(struct db_state *state)
{
    char file[PATH_MAX];
    snprintf(file, sizeof(file), "%s/prefix.lisp", state->path.stars);

    struct config config = {0};
    struct reader *in = config_read(&config, file);

    struct db_names names = {0};
    db_names_read(in, &names);

    db_file_writef(&state->files.stars_prefix,
            "stars_prefix_begin(%zu)\n", names.len);

    for (size_t i = 0; i < names.len; ++i) {
        db_file_writef(&state->files.stars_prefix,
                "  stars_prefix(%zu, \"%s\")\n", i, names.list[i].c);
    }

    db_file_write(&state->files.stars_prefix, "stars_prefix_end()\n");
}

static void db_gen_suffix(struct db_state *state)
{
    char file[PATH_MAX];
    snprintf(file, sizeof(file), "%s/suffix.lisp", state->path.stars);

    struct config config = {0};
    struct reader *in = config_read(&config, file);

    while (!reader_peek_eof(in)) {
        struct db_names names = {0};
        db_names_read(in, &names);

        db_file_writef(&state->files.stars_suffix,
                "stars_suffix_begin(\"%s\", %zu)\n",
                names.name.c, names.len);

        for (size_t i = 0; i < names.len; ++i) {
            db_file_writef(&state->files.stars_suffix,
                    "  stars_suffix(%zu, \"%s\")\n", i, names.list[i].c);
        }

        db_file_write(&state->files.stars_suffix, "stars_suffix_end()\n\n");
    }
}


// -----------------------------------------------------------------------------
// rolls
// -----------------------------------------------------------------------------

enum db_roll_type
{
    roll_one,
    roll_rng,
    roll_one_of,
    roll_all_of,
};

struct db_rolls_range
{
    enum db_roll_type type;
    struct symbol min, max;
    uint16_t count;
};

struct db_rolls
{
    size_t len;
    struct db_rolls_range list[16];
};

static void db_rolls_read(struct reader *in, struct db_rolls *rolls)
{
    while (!reader_peek_close(in)) {
        struct db_rolls_range *roll = rolls->list + rolls->len;
        reader_open(in);

        static struct reader_table types[] =  {
            { .str = "one", .value = roll_one },
            { .str = "rng", .value = roll_rng },
            { .str = "one-of", .value = roll_one_of },
            { .str = "all-of", .value = roll_all_of },
        };
        roll->type = reader_symbol_table(in, types, array_len(types));

        roll->min = symbol_to_enum(reader_symbol(in));

        if (roll->type != roll_one)
            roll->max = symbol_to_enum(reader_symbol(in));
        else roll->max = roll->min;

        roll->count = reader_word(in);

        reader_close(in);
        rolls->len++;
    }
}

static void db_gen_rolls(struct db_state *state)
{
    char file[PATH_MAX];
    snprintf(file, sizeof(file), "%s/rolls.lisp", state->path.stars);

    struct config config = {0};
    struct reader *in = config_read(&config, file);

    while (!reader_peek_eof(in)) {
        reader_open(in);

        struct symbol name = reader_symbol(in);
        struct db_rolls rolls = {0};
        size_t weight = 0;
        size_t hue = 0;

        while (!reader_peek_close(in)) {
            reader_open(in);

            struct symbol key = reader_symbol(in);
            hash_val hash = symbol_hash(&key);

            if (hash == symbol_hash_c("hue")) {
                hue = reader_word(in);
                reader_close(in);
            }
            else if (hash == symbol_hash_c("weight")) {
                weight = reader_word(in);
                reader_close(in);
            }
            else if (hash == symbol_hash_c("rolls")) {
                db_rolls_read(in, &rolls);
                reader_close(in);
            }
            else {
                reader_err(in, "unknown roll key '%s'", key.c);
                reader_goto_close(in);
            }
        }
        reader_close(in);

        db_file_writef(&state->files.stars_rolls,
                "stars_rolls_begin(\"%s\", %zu, %zu, %zu)\n",
                name.c, weight, hue, rolls.len);

        for (size_t i = 0; i < rolls.len; ++i) {
            struct db_rolls_range *roll = rolls.list + i;

            const char *type_str = "nil";
            switch (roll->type) {
            case roll_one: { type_str = "one"; break; }
            case roll_rng: { type_str = "rng"; break; }
            case roll_one_of: { type_str = "one_of"; break; }
            case roll_all_of: { type_str = "all_of"; break; }
            default: { assert(false); }
            }

            db_file_writef(&state->files.stars_rolls,
                    "  stars_rolls(%zu, %s, %s, %s, %u)\n",
                    i, type_str, roll->min.c, roll->max.c, roll->count);
        }

        db_file_write(&state->files.stars_rolls, "stars_rolls_end()\n\n");
    }
}


// -----------------------------------------------------------------------------
// stars
// -----------------------------------------------------------------------------

static void db_gen_stars(struct db_state *state)
{
    db_gen_prefix(state);
    db_gen_suffix(state);
    db_gen_rolls(state);
}
