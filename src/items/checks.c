/* checks.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "game/world.h"
#include "game/tech.h"
#include "items/io.h"


// -----------------------------------------------------------------------------
// checks
// -----------------------------------------------------------------------------

inline bool im_check_args(
        struct chunk *chunk, id_t id, enum io io, size_t len, size_t exp)
{
    if (likely(len >= exp)) return true;
    chunk_log(chunk, id, io, IOE_MISSING_ARG);
    return false;
}

inline bool im_check_known(
        struct chunk *chunk, id_t id, enum io io, enum item item)
{
    struct tech *tech = chunk_tech(chunk);
    if (tech_known(tech, item)) return true;

    chunk_log(chunk, id, io, IOE_A0_UNKNOWN);
    return false;
}
