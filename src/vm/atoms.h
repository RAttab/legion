/* atoms.h
   Rémi Attab (remi.attab@gmail.com), 28 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// vm_atoms
// -----------------------------------------------------------------------------

enum { vm_atom_cap = 16; };
typedef char atom_t[vm_atom_cap];

void vm_atoms_init(void);

word_t vm_atom(const atom_t *);
bool vm_atoms_set(const atom_t *, word_t id);
bool vm_atoms_str(word_t, atom_t *dst);
