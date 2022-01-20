/* config.c
   Rémi Attab (remi.attab@gmail.com), 15 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "vm/symbol.h"
#include "game/user.h"
#include "utils/config.h"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

static void config_server(const char *path)
{
    struct user user = {
        .id = 0,
        .name = make_symbol("admin"),
        .access = ((uset_t) -1ULL),
        .public = token(),
        .private = token(),

    };

    struct config config = {0};
    struct writer *out = config_write(&config, path);

    {
        writer_open(out);
        writer_symbol_str(out, "users");
        writer_field(out, "server", u64, token());

        writer_open_nl(out);
        writer_symbol(out, &user.name);
        user_write(&user, out),
        writer_close(out);

        writer_close(out);
    }

    config_close(&config);
}

static void config_client(
        const char *path,
        const struct symbol *name,
        token_t auth)
{
    struct config config = {0};
    struct writer *out = config_write(&config, path);

    writer_open(out);
    writer_symbol_str(out, "client");
    writer_field(out, "server", u64, auth);

    writer_open_nl(out);
    writer_symbol(out, name);
    writer_close(out);

    writer_close(out);
    config_close(&config);
}

bool config_run(
        const char *type,
        const char *path,
        const struct symbol *name,
        token_t auth)
{
    if (strcmp(type, "server") == 0) { config_server(path); return true; }
    if (strcmp(type, "client") == 0) { config_client(path, name, auth); return true; }

    usage(1, "unknown config type argument");
    return false;
}
