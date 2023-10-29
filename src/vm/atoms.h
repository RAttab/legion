/* atoms.h
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"

struct symbol;
struct save;
struct ack;

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

enum : vm_word {
    atom_nil = 0x00,

    atom_ns_io   = 0x01000000,
    atom_ns_ioe  = 0x02000000,
    atom_ns_user = 0x80000000,
};


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

struct atoms;

struct atoms *atoms_new(void);
void atoms_free(struct atoms *);

void atoms_save(struct atoms *, struct save *);
struct atoms *atoms_load(struct save *);

void atoms_save_delta(struct atoms *, struct save *, const struct ack *);
bool atoms_load_delta(struct atoms *, struct save *, struct ack *);

bool atoms_set(struct atoms *, const struct symbol *, vm_word id);
vm_word atoms_get(struct atoms *, const struct symbol *);
vm_word atoms_make(struct atoms *, const struct symbol *);
bool atoms_str(struct atoms *, vm_word id, struct symbol *dst);
vm_word atoms_parse(struct atoms *, const char *str, size_t len);

struct vec64 *atoms_list(struct atoms *);
