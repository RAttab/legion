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
    *dst++; len--;

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
    [ITEM_FRAME]         = "fram",
    [ITEM_GEAR]          = "gear",
    [ITEM_FUEL]          = "fuel",
    [ITEM_BONDING]       = "bond",
    [ITEM_CIRCUIT]       = "circ",
    [ITEM_NEURAL]        = "neur",
    [ITEM_SERVO]         = "serv",
    [ITEM_THRUSTER]      = "thrs",
    [ITEM_PROPULSION]    = "prop",
    [ITEM_PLATE]         = "plat",
    [ITEM_SHIELDING]     = "shld",
    [ITEM_HULL_I]        = "hull",
    [ITEM_CORE]          = "core",
    [ITEM_MATRIX]        = "mtrx",
    [ITEM_DATABANK]      = "dbnk",

    // Logistics
    [ITEM_WORKER]         = "work",
    [ITEM_SHUTTLE_S]      = "sh.1",
    [ITEM_SHUTTLE_M]      = "sh.2",
    [ITEM_SHUTTLE_F]      = "sh.3",

    // Actives
    [ITEM_DEPLOY]        = "depl",
    [ITEM_EXTRACT_I]     = "ex.1",
    [ITEM_EXTRACT_II]    = "ex.2",
    [ITEM_EXTRACT_III]   = "ex.3",
    [ITEM_PRINTER_I]     = "pr.1",
    [ITEM_PRINTER_II]    = "pr.2",
    [ITEM_PRINTER_III]   = "pr.3",
    [ITEM_ASSEMBLER_I]   = "as.1",
    [ITEM_ASSEMBLER_II]  = "as.2",
    [ITEM_ASSEMBLER_III] = "as.3",
    [ITEM_STORAGE]       = "stor",
    [ITEM_DB_I]          = "db.1",
    [ITEM_DB_II]         = "db.2",
    [ITEM_DB_III]        = "db.3",
    [ITEM_BRAIN_I]       = "br.1",
    [ITEM_BRAIN_II]      = "br.2",
    [ITEM_BRAIN_III]     = "br.3",
    [ITEM_LEGION_I]      = "lg.1",
    [ITEM_LEGION_II]     = "lg.2",
    [ITEM_LEGION_III]    = "lg.3",

    0,
};

size_t item_str(item_t item, size_t len, char *dst)
{
    assert(len <= 4);
    assert(item < array_len(item_str_table));

    const char *str = item_str_table[item];
    assert(str);

    memcpy(dst, str, 4);
    return 4;
}
