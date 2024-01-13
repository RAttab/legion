/* db_man.c
   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct db_man
{
    size_t len, cap;
    char paths[][PATH_MAX];
};

static void db_man_path(
        struct db_state *state, const char *dir, struct db_man *man)
{
    struct dir_it *it = dir_it(dir);

    while (dir_it_next(it)) {
        const char *path = dir_it_path(it);

        if (path_is_dir(path)) {
            db_man_path(state, path, man);
            continue;
        }

        if (!path_is_file(path)) continue;
        if (!str_ends_with(path, ".lm")) continue;
        if (str_starts_with(dir_it_name(it), "_")) continue;

        assert(man->len < man->cap);
        strncpy(man->paths[man->len], path, sizeof(man->paths[0]));
        man->len++;
    }
}

static void db_gen_man(struct db_state *state)
{
    db_file_write(&state->files.man,
            "\t.global db_man_list\n"
            "\t.type db_man_list, STT_OBJECT\n"
            "\t.balign 64\n"
            "db_man_list:\n\n");

    constexpr size_t cap = 128;
    struct db_man *man = mem_struct_alloc_t(man, man->paths[0], cap);
    *man = (struct db_man) { .len = 0, .cap = cap };

    db_man_path(state, state->path.man, man);

    int cmp(const void *lhs, const void *rhs)
    {
        return strncmp(lhs, rhs, PATH_MAX);
    }
    qsort(man->paths, man->len, sizeof(man->paths[0]), cmp);

    for (size_t i = 0; i < man->len; ++i) {
        const char *path = man->paths[i];
        db_file_writef(&state->files.man,
                "\t.byte 0xFF\n"
                "\t.4byte %zu\n"
                "\t.4byte %zu\n"
                "\t.asciz \"%s\"\n"
                "\t.incbin \"%s\"\n\n",
                strlen(path) + 1, file_len_p(path), path, path);
    }

    mem_free(man);

    db_file_write(&state->files.man,
            "\t.global db_man_list_end\n"
            "\t.type db_man_list_end, STT_OBJECT\n"
            "db_man_list_end:\n"
            "\t.byte 0x00\n"
            "\t.4byte 0\n"
            "\t.4byte 0\n\n");
}
