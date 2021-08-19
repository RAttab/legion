/* types.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

typedef uint16_t loops_t;
enum { loops_inf = UINT16_MAX };
inline loops_t loops_io(word_t loops)
{
    return loops > 0 && loops < loops_inf ? loops : loops_inf;
}
