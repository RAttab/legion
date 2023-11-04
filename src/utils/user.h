/* user.h
   Rémi Attab (remi.attab@gmail.com), 01 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/htable.h"
#include "utils/symbol.h"

struct reader;
struct writer;
struct save;

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

typedef uint8_t user_id;
enum : size_t { user_max = 64 };
enum : user_id { user_admin = 0 };


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

typedef uint64_t user_token;
user_token make_user_token(void);


// -----------------------------------------------------------------------------
// uset
// -----------------------------------------------------------------------------

typedef uint64_t user_set;

inline user_set user_to_set(user_id id)
{
    assert(id < 64);
    return 1ULL << id;
}

inline bool user_set_test(user_set list, user_id id)
{
    return list & user_to_set(id);
}

inline user_set user_set_nil(void) { return 0UL; }
inline user_set user_set_all(void) { return -1UL; }

// -----------------------------------------------------------------------------
// user
// -----------------------------------------------------------------------------

struct user
{
    user_id id;
    struct symbol name;
    user_set access;
    user_token public, private;
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
    user_token server;
    user_set avail;
    struct htable ids;
    struct htable names;
    struct htable grant;
};

void users_init(struct users *);
void users_free(struct users *);

struct user *users_create(struct users *, const struct symbol *);
const struct user *users_name(struct users *, const struct symbol *);
const struct user *users_id(struct users *, user_id);

bool users_auth_server(struct users *, user_token);
const struct user *users_auth_user(struct users *, user_id, user_token);
bool users_grant(struct users *, user_id, user_token);

void users_write(const struct users *, struct writer *);
void users_read(struct users *, struct reader *);
