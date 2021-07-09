/* item.c
   Rémi Attab (remi.attab@gmail.com), 06 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/item.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

inline size_t id_str(id_t id, size_t len, char *dst)
{
    assert(len <= id_str_len);

    size_t ret = item_str(id_item(id), len, dst);
    dst += ret; len -= ret;

    *dst = '.';
    dst++; len--;

    str_utox(id_bot(id), dst, len);
    return id_str_len;
}


// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------

static const char *item_str_table[] =
{
    [ITEM_NIL] = "null",

    // Natural
    [ITEM_ELEM_A] = "el.A",
    [ITEM_ELEM_B] = "el.B",
    [ITEM_ELEM_C] = "el.C",
    [ITEM_ELEM_D] = "el.D",
    [ITEM_ELEM_E] = "el.E",
    [ITEM_ELEM_F] = "el.F",
    [ITEM_ELEM_G] = "el.G",
    [ITEM_ELEM_H] = "el.H",
    [ITEM_ELEM_I] = "el.I",
    [ITEM_ELEM_J] = "el.J",
    [ITEM_ELEM_K] = "el.K",

    // Synth
    [ITEM_ELEM_L] = "el.L",
    [ITEM_ELEM_M] = "el.M",
    [ITEM_ELEM_N] = "el.N",
    [ITEM_ELEM_O] = "el.O",
    [ITEM_ELEM_P] = "el.P",
    [ITEM_ELEM_Q] = "el.Q",
    [ITEM_ELEM_R] = "el.R",
    [ITEM_ELEM_S] = "el.S",
    [ITEM_ELEM_T] = "el.T",
    [ITEM_ELEM_U] = "el.U",
    [ITEM_ELEM_V] = "el.V",
    [ITEM_ELEM_W] = "el.W",
    [ITEM_ELEM_X] = "el.X",
    [ITEM_ELEM_Y] = "el.Y",
    [ITEM_ELEM_Z] = "el.Z",

    // Passives
    [ITEM_FRAME]      = "fram",
    [ITEM_GEAR]       = "gear",
    [ITEM_FUEL]       = "fuel",
    [ITEM_BONDING]    = "bond",
    [ITEM_CIRCUIT]    = "circ",
    [ITEM_NEURAL]     = "neur",
    [ITEM_SERVO]      = "serv",
    [ITEM_THRUSTER]   = "thrs",
    [ITEM_PROPULSION] = "prop",
    [ITEM_PLATE]      = "plat",
    [ITEM_SHIELDING]  = "shld",
    [ITEM_HULL_1]     = "hull",
    [ITEM_CORE]       = "core",
    [ITEM_MATRIX]     = "mtrx",
    [ITEM_DATABANK]   = "dbnk",

    // Logistics
    [ITEM_WORKER]    = "work",
    [ITEM_SHUTTLE_1] = "sh.1",
    [ITEM_SHUTTLE_2] = "sh.2",
    [ITEM_SHUTTLE_3] = "sh.3",

    // Actives
    [ITEM_DEPLOY]      = "depl",
    [ITEM_EXTRACT_1]   = "ex.1",
    [ITEM_EXTRACT_2]   = "ex.2",
    [ITEM_EXTRACT_3]   = "ex.3",
    [ITEM_PRINTER_1]   = "pr.1",
    [ITEM_PRINTER_2]   = "pr.2",
    [ITEM_PRINTER_3]   = "pr.3",
    [ITEM_ASSEMBLY_1]  = "as.1",
    [ITEM_ASSEMBLY_2]  = "as.2",
    [ITEM_ASSEMBLY_3]  = "as.3",
    [ITEM_STORAGE]     = "stor",
    [ITEM_DB_1]        = "db.1",
    [ITEM_DB_2]        = "db.2",
    [ITEM_DB_3]        = "db.3",
    [ITEM_BRAIN_1]     = "br.1",
    [ITEM_BRAIN_2]     = "br.2",
    [ITEM_BRAIN_3]     = "br.3",
    [ITEM_LEGION_1]    = "lg.1",
    [ITEM_LEGION_2]    = "lg.2",
    [ITEM_LEGION_3]    = "lg.3",

    0,
};

size_t item_str(enum item item, size_t len, char *dst)
{
    assert(len >= 4);
    assert(item < array_len(item_str_table));

    const char *str = item_str_table[item];
    assert(str);

    memcpy(dst, str, 4);
    return 4;
}
