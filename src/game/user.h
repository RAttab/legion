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

typedef uint16_t uid_t;


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

typedef uint64_t token_t;

token_t token(void);


// -----------------------------------------------------------------------------
// uset
// -----------------------------------------------------------------------------

typedef uint64_t uset_t;

inline uset_t uid_to_uset(uid_t id)
{
    assert(id < 64);
    return 1ULL << id;
}

inline bool uset_test(uset_t list, uid_t id)
{
    return list & uid_to_uset(id);
}


// -----------------------------------------------------------------------------
// user
// -----------------------------------------------------------------------------

struct user
{
    uid_t id;
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

void users_init(struct users *);
void users_free(struct users *);

struct user *users_create(struct users *, word_t atom);
struct user *users_atom(struct users *, word_t atom);
struct user *users_id(struct users *, uid_t);

bool users_auth_server(struct users *, token_t);
bool users_auth_user(struct users *, uid_t, token_t);
bool users_grant(struct users *, uid_t, token_t);

void users_write(const struct users *, struct atoms *, struct writer *);
void users_read(struct users *, struct atoms *, struct reader *);
