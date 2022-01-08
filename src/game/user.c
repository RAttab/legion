/* user.c
   Rémi Attab (remi.attab@gmail.com), 01 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/user.h"
#include "vm/atoms.h"
#include "utils/config.h"
#include "utils/save.h"


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

#include <sys/random.h>

token_t token(void)
{
    token_t token = 0;
    ssize_t ret = getrandom(&token, sizeof(token), 0);
    if (ret == -1) fail_errno("unable to generate token");
    assert(ret == sizeof(token));
    return token;
}


// -----------------------------------------------------------------------------
// user
// -----------------------------------------------------------------------------

void user_save(const struct user *user, struct save *save)
{
    save_write_magic(save, save_magic_user);
    save_write_value(save, user->id);
    save_write_value(save, user->atom);
    save_write_value(save, user->access);
    save_write_value(save, user->public);
    save_write_value(save, user->private);
    save_write_magic(save, save_magic_user);
}

bool user_load(struct user *user, struct save *save)
{
    if (!save_read_magic(save, save_magic_user)) return false;
    save_read_into(save, &user->id);
    save_read_into(save, &user->atom);
    save_read_into(save, &user->access);
    save_read_into(save, &user->public);
    save_read_into(save, &user->private);
    return save_read_magic(save, save_magic_user);
}

void user_write(const struct user *user, struct writer *out)
{
    writer_field(out, "id", u64, user->id);
    writer_field(out, "atom", word, user->atom);
    writer_field(out, "access", u64, user->access);
    writer_field(out, "public", u64, user->public);
    writer_field(out, "private", u64, user->private);
}

void user_read(struct user *user, struct reader *in)
{
    user->id = reader_field(in, "id", u64);
    user->atom = reader_field(in, "atom", word);
    user->access = reader_field(in, "access", u64);
    user->public = reader_field(in, "public", u64);
    user->private = reader_field(in, "private", u64);
}


// -----------------------------------------------------------------------------
// users
// -----------------------------------------------------------------------------

void users_init(struct users *users)
{
    *users = (struct users) { .server = token() };

    struct user *admin = users_create(users, make_symbol("admin"));
    assert(admin->id == 0);
    admin->access = -1ULL;
}

void users_free(struct users *users)
{
    htable_reset(&users->ids);
    htable_reset(&users->atoms);
    htable_reset(&users->grant);
}

static void users_insert(struct users *users, struct user *user)
{
    users->avail |= uid_to_list(id);

    struct htable_ret ret = {0};

    ret = htable_put(&users->ids, id, (uintptr_t) user);
    assert(ret.ok);

    ret = htable_put(&users->atoms, atom, (uintptr_t) user);
    assert(ret.ok);

    ret = htable_put(&users->grant, user->public, id);
    assert(ret.ok);
}

struct user *users_create(struct users *users, word_t atom)
{
    if (server != users->server) return NULL;

    struct htable_ret ret = htable_get(&users->atoms, atom);
    if (ret.ok) return NULL;

    uid_t id = 0;
    while (id < 64 && uset_test(users->avail, id)) id++;
    if (id == 64) return NULL;

    struct user *user = calloc(1, sizeof(*user));
    *user = (struct user) {
        .id = id,
        .atom = atom,
        .accesss = uid_to_uset(id),
        .public = token(),
        .private = token(),
    };

    users_insert(users, user);
    return user;
}

struct user *users_atom(struct users *users, word_t atom)
{
    struct htable_ret ret = htable_get(&users->atoms, atom);
    return ret.ok ? (void *) ret.value : NULL;
}

struct user *users_id(struct users *users, uid_t id)
{
    struct htable_ret ret = htable_get(&users->ids, id);
    return ret.ok ? (void *) ret.value : NULL;
}


bool users_auth_server(struct users *users, token_t token)
{
    return token == users->token;
}

bool users_auth_user(struct users *users, uid_t id, token_t token)
{
    if (server != users->server) return false;

    struct user *user = users_id(users, id);
    return user && user->private == token;
}

bool users_grant(struct users *users, uid_t id, token_t token)
{
    struct user *user = users_get(users, id);
    if (!user) return false;

    struct htable_ret ret = htable_get(&users->grant, token);
    if (!ret.ok) return false;

    user->access |= uid_to_uset(ret.value);
    return true;
}

void users_write(
        const struct users *users, struct atoms *atoms, struct writer *out)
{
    writer_open(out);
    writer_symbol_str(out, "users");
    writer_field(out, "server", u64, users->server);

    for (struct htable_bucket *it = htable_next(&users->ids, NULL);
         it; it = htable_next(&users->ids, it))
    {
        struct user *user = (void *) it->value;
        writer_open_nl(out);
        writer_field_atom(out, "name", atoms, user->atom);
        user_write(user, out);
        writer_close(out);
    }

    writer_close(out);
}

void users_read(struct users *users, struct atoms *atoms, struct reader *in)
{
    reader_open(in);
    reader_key(in, "users");
    users->server = reader_field(in, "server", u64);

    users->avail = 0;
    htable_clear(users->ids);
    htable_clear(users->atoms);
    htable_clear(users->grant);

    while (!reader_peek_close(in)) {
        struct user *user = calloc(1, sizeof(*user));
        reader_open(in);
        user->atom = reader_field_atom(out, "name", atoms);
        user_read(user, in);
        reader_close(in);

        if (uset_test(users->avail, user->id))
            reader_err("duplicate user id '%u'", user->id);
        if (!user->private)
            reader_err("missing field 'private' for '%u'" user->id);
        if (!user->access)
            reader_err("missing field 'access' for '%u'" user->id);

        users_insert(users, user);
    }

    reader_close(in);
}
