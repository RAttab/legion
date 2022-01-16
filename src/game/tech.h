/* tech.h
   Rémi Attab (remi.attab@gmail.com), 31 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/tape.h"
#include "utils/htable.h"

struct save;


// -----------------------------------------------------------------------------
// tech
// -----------------------------------------------------------------------------

struct tech
{
    struct tape_set known;
    struct tape_set learned;
    struct htable research;
};

void tech_free(struct tech *);

void tech_save(const struct tech *, struct save *);
bool tech_load(struct tech *, struct save *);

bool tech_known(const struct tech *, enum item);
struct tape_set tech_known_list(const struct tech *);

void tech_learn(struct tech *, enum item);
bool tech_learned(const struct tech *, enum item);
struct tape_set tech_learned_list(const struct tech *);

uint64_t tech_learned_bits(const struct tech *, enum item);
void tech_learn_bit(struct tech *, enum item, uint8_t bit);

void tech_populate(struct tech *);
