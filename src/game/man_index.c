/* man_index.c
   Rémi Attab (remi.attab@gmail.com), 23 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/

// included in man.c


// -----------------------------------------------------------------------------
// man_page_index
// -----------------------------------------------------------------------------

static bool man_page_index(
        struct man_page *page, struct toc *toc, struct atoms *atoms)
{
    struct man_parser parser = {
        .ok = true,
        .in = {
            .page = page,
            .it = page->file.ptr,
            .end = page->file.ptr + page->file.len,
            .line = 0, .col = 0,
        },
    };

    man_section sections = 0;
    struct man_path path = {0};
    uint8_t path_title = 0, path_section = 0;

    struct man_token token = {0};
    while ((token = man_parser_next(&parser)).type != man_token_eof) {
        if (token.type != man_token_markup) continue;

        switch (man_token_markup_type(&token))
        {

        case man_markup_title: {
            if (path_title) {
                man_err(&parser, "unable to have multiple titles within a page");
                continue;
            }

            struct man_token str = man_parser_until_close(&parser);
            if (str.type != man_token_line) continue;

            if (!man_path_append(&path, str.it, str.len)) {
                man_err(&parser, "invalid title header");
                continue;
            }

            struct link link = make_link(page->page, 0);
            man_index_path(path.str, path.len, link);
            toc_path(toc, path)->link = link;

            path_title = path.len;
            break;
        }

        case man_markup_section: {
            if (!path_title) man_err(&parser, "missing title header");

            struct man_token str = man_parser_until_close(&parser);

            path.len = path_title;
            if (!man_path_append(&path, str.it, str.len)) {
                man_err(&parser, "invalid section header");
                continue;
            }

            struct link link = make_link(page->page, ++sections);
            man_index_path(path.str, path.len, link);
            toc_path(toc, path)->link = link;

            path_section = path.len;
            break;
        }

        case man_markup_topic: {
            if (!path_section) man_err(&parser, "missing section header");

            struct man_token str = man_parser_until_close(&parser);

            path.len = path_section;
            if (!man_path_append(&path, str.it, str.len)) {
                man_err(&parser, "invalid topic header");
                continue;
            }

            struct link link = make_link(page->page, ++sections);
            man_index_path(path.str, path.len, link);
            toc_path(toc, path)->link = link;
            break;
        }

        case man_markup_item: {
            if (!path_title) man_err(&parser, "item must be after title header");
            if (path_section) man_err(&parser, "item must be before any section header");

            struct man_token str = man_parser_until_close(&parser);
            struct symbol sym = make_symbol_len(str.it, str.len);
            vm_word word = atoms_get(atoms, &sym);

            if (word <= item_nil || word >= items_max) {
                man_err(&parser, "invalid item atom: %.*s -> %lx",
                        str.len, str.it, word);
                continue;
            }

            toc_path(toc, path)->item = word;
            page->item = word;
            break;
        }

        default: { continue; }
        }
    }

    return parser.ok;
}
