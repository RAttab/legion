/* user.h
   Rémi Attab (remi.attab@gmail.com), 01 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "utils/htable.h"

struct atoms;
struct reader;
struct writer;
struct save;


// -----------------------------------------------------------------------------
// uid
// -----------------------------------------------------------------------------

typedef uint8_t user_t;
enum { user_max = 64 };
enum { user_admin = 0 };


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

typedef uint64_t token_t;

token_t token(void);


// -----------------------------------------------------------------------------
// uset
// -----------------------------------------------------------------------------

typedef uint64_t uset_t;

inline uset_t user_to_uset(user_t id)
{
    assert(id < 64);
    return 1ULL << id;
}

inline bool uset_test(uset_t list, user_t id)
{
    return list & user_to_uset(id);
}

inline uset_t uset_nil(void) { return 0UL; }
inline uset_t uset_all(void) { return -1UL; }

// -----------------------------------------------------------------------------
// user
// -----------------------------------------------------------------------------

struct user
{
    user_t id;
    word_t atom;
    uset_t access;
    token_t public, private;
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
    token_t server;
    uset_t avail;
    struct htable ids;
    struct htable atoms;
    struct htable grant;
};

void users_init(struct users *, struct atoms *);
void users_free(struct users *);

struct user *users_create(struct users *, word_t atom);
struct user *users_atom(struct users *, word_t atom);
struct user *users_id(struct users *, user_t);

bool users_auth_server(struct users *, token_t);
struct user *users_auth_user(struct users *, user_t, token_t);
bool users_grant(struct users *, user_t, token_t);

void users_write(const struct users *, struct atoms *, struct writer *);
void users_read(struct users *, struct atoms *, struct reader *);
