/* db_parse.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// parse
// -----------------------------------------------------------------------------

static void db_parse_atoms(struct db_state *state, const char *path)
{
    struct config config = {0};
    struct reader *in = config_read(&config, path);

    while (!reader_peek_eof(in)) {
        reader_open(in);

        struct db_info *info = &state->info->vals[state->info->len++];
        *info = (struct db_info) {
            .name = reader_symbol(in),
            .order = order_nil,
            .atom = 0,
        };

        reader_open(in);
        reader_symbol_str(in, "info");

        while (!reader_peek_close(in)) {
            reader_open(in);

            struct symbol field = reader_symbol(in);
            uint64_t hash = symbol_hash(&field);

            if (hash == symbol_hash_c("type")) {
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
            }

            else if (hash == symbol_hash_c("list")) {
                struct symbol list = reader_symbol(in);
                uint64_t hash = symbol_hash(&list);
                if (hash == symbol_hash_c("nil"))   info->list = list_nil;
                else if (hash == symbol_hash_c("control")) info->list = list_control;
                else if (hash == symbol_hash_c("factory"))  info->list = list_factory;
                else reader_err(in, "unkown info list: %s", list.c);
                reader_close(in);
            }

            else if (hash == symbol_hash_c("order")) {
                struct symbol order = reader_symbol(in);
                uint64_t hash = symbol_hash(&order);
                if (hash == symbol_hash_c("nil"))   info->order = order_nil;
                else if (hash == symbol_hash_c("first")) info->order = order_first;
                else if (hash == symbol_hash_c("last"))  info->order = order_last;
                else reader_err(in, "unkown info order: %s", order.c);
                reader_close(in);
            }

            else if (hash == symbol_hash_c("config")) {
                info->config = reader_symbol(in);
                reader_close(in);
            }

            else reader_goto_close(in);

        }
        assert(info->type != im_type_nil);

        reader_close(in); // info

        // skip the rest of the object
        reader_goto_close(in);
    }

    config_close(&config);
}


