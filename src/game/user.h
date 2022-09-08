/* user.h
   Rémi Attab (remi.attab@gmail.com), 01 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/symbol.h"
#include "utils/htable.h"

struct reader;
struct writer;
struct save;


// -----------------------------------------------------------------------------
// uid
// -----------------------------------------------------------------------------

typedef uint8_t user;
enum { user_max = 64 };
enum { user_admin = 0 };


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

typedef uint64_t token;

token make_token(void);


// -----------------------------------------------------------------------------
// uset
// -----------------------------------------------------------------------------

typedef uint64_t uset;

inline uset user_to_uset(user id)
{
    assert(id < 64);
    return 1ULL << id;
}

inline bool uset_test(uset list, user id)
{
    return list & user_to_uset(id);
}

inline uset uset_nil(void) { return 0UL; }
inline uset uset_all(void) { return -1UL; }

// -----------------------------------------------------------------------------
// user
// -----------------------------------------------------------------------------

struct user
{
    user id;
    struct symbol name;
    uset access;
    token public, private;
};

void user_save(const struct user *, struct save *);
bool user_load(struct user *, struct save *);

void user_write(const struct user *, struct writer *);
void user_read(struct user *, struct reader *);


// -----------------------------------------------------------------------------
// users
// -----------------------------------------------------------------------------

struct users
{
    token server;
    uset avail;
    struct htable ids;
    struct htable names;
    struct htable grant;
};

void users_init(struct users *);
void users_free(struct users *);

struct user *users_create(struct users *, const struct symbol *);
const struct user *users_name(struct users *, const struct symbol *);
const struct user *users_id(struct users *, user);

bool users_auth_server(struct users *, token);
const struct user *users_auth_user(struct users *, user, token);
bool users_grant(struct users *, user, token);

void users_write(const struct users *, struct writer *);
void users_read(struct users *, struct reader *);
