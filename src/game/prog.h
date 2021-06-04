/* program.h
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// prog
// -----------------------------------------------------------------------------

struct prog;

typedef uint8_t prog_id_t;

enum prog_state
{
    prog_eof = 0,
    prog_input,
    prog_output,
};

struct prog_ret
{
    enum prog_state state;
    item_t item;
};

void prog_load();
const struct prog *prog_fetch(prog_id_t prog);

prog_id_t prog_id(const struct prog *);
item_t prog_host(const struct prog *);
struct prog_ret prog_at(const struct prog *, uint16_t index);
