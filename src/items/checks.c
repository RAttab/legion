/* checks.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// checks
// -----------------------------------------------------------------------------

inline bool im_check_args(
        struct chunk *chunk, im_id id, enum io io, size_t len, size_t exp)
{
    if (likely(len >= exp)) return true;
    chunk_log(chunk, id, io, ioe_missing_arg);
    return false;
}

inline bool im_check_known(
        struct chunk *chunk, im_id id, enum io io, enum item item)
{
    const struct tech *tech = chunk_tech(chunk);
    if (tech_known(tech, item)) return true;

    chunk_log(chunk, id, io, ioe_a0_unknown);
    return false;
}
