/* atoms.h
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"

struct symbol;
struct save;

// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

struct atoms;

struct atoms *atoms_new(void);
void atoms_free(struct atoms *);

struct atoms *atoms_load(struct save *);
bool atoms_load_into(struct atoms *, struct save *);
void atoms_save(struct atoms *, struct save *);

bool atoms_set(struct atoms *, const struct symbol *, word_t id);
word_t atoms_get(struct atoms *, const struct symbol *);
word_t atoms_make(struct atoms *, const struct symbol *);
bool atoms_str(struct atoms *, word_t id, struct symbol *dst);
word_t atoms_parse(struct atoms *, const char *str, size_t len);

struct vec64 *atoms_list(struct atoms *);
