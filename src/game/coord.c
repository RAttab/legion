/* coord.c
   Rémi Attab (remi.attab@gmail.com), 12 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

size_t coord_str(struct coord coord, char *str, size_t len)
{
    assert(len >= coord_str_len);

    size_t i = 0;
    str[i++] = str_hexchar(coord.x >> 28);
    str[i++] = str_hexchar(coord.x >> 24);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.x >> 20);
    str[i++] = str_hexchar(coord.x >> 16);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.x >> 12);
    str[i++] = str_hexchar(coord.x >> 8);
    str[i++] = str_hexchar(coord.x >> 4);
    str[i++] = str_hexchar(coord.x >> 0);
    str[i++] = ' ';
    str[i++] = 'x';
    str[i++] = ' ';
    str[i++] = str_hexchar(coord.y >> 28);
    str[i++] = str_hexchar(coord.y >> 24);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.y >> 20);
    str[i++] = str_hexchar(coord.y >> 16);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.y >> 12);
    str[i++] = str_hexchar(coord.y >> 8);
    str[i++] = str_hexchar(coord.y >> 4);
    str[i++] = str_hexchar(coord.y >> 0);

    assert(i == coord_str_len);
    return coord_str_len;
}

// -----------------------------------------------------------------------------
// scale
// -----------------------------------------------------------------------------

inline coord_scale coord_scale_inc(coord_scale scale, int dir)
{
    if (scale == coord_scale_min && dir < 0) return scale;

    uint64_t delta = (1 << (u64_log2(scale) - coord_scale_bits));
    return dir < 0 ? scale - delta : scale + delta;
}


size_t coord_scale_str(coord_scale scale, char *str, size_t len)
{
    assert(len >= coord_scale_str_len);

    size_t i = 0;
    size_t msb = u64_log2(scale);
    uint64_t top = scale < coord_scale_min ? 0 : msb - coord_scale_bits;
    uint64_t bot = scale < coord_scale_min ? (uint64_t)scale :
        (((uint64_t) scale) & ((1 << msb) - 1)) >> (msb - coord_scale_bits);

    str[i++] = 'x';
    str[i++] = str_hexchar(top >> 4);
    str[i++] = str_hexchar(top >> 0);
    str[i++] = '.';
    str[i++] = str_hexchar(bot >> 4);
    str[i++] = str_hexchar(bot >> 0);

    assert(i == coord_scale_str_len);
    return coord_scale_str_len;
}
