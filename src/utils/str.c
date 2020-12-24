/* str.c
   Rémi Attab (remi.attab@gmail.com), 09 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "str.h"


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

size_t str_utoa(uint64_t val, char *dst, size_t len)
{
    size_t i = 0;
    for (; i < len; ++i, val /= 10)
        dst[len-i-1] = '0' + (val % 10);
    return i;
}

size_t str_utox(uint64_t val, char *dst, size_t len)
{
    size_t i = 0;
    for (; i < len; ++i, val >>= 4)
        dst[len-i-1] = str_hexchar(val);
    return i;
}

static char str_keycode_shift_map[] = {
    ['a'] = 'A', ['b'] = 'B', ['c'] = 'C', ['d'] = 'D', ['e'] = 'E',
    ['f'] = 'F', ['g'] = 'G', ['h'] = 'H', ['i'] = 'I', ['j'] = 'J',
    ['k'] = 'K', ['l'] = 'L', ['m'] = 'M', ['n'] = 'N', ['o'] = 'O',
    ['p'] = 'P', ['q'] = 'Q', ['r'] = 'R', ['s'] = 'S', ['t'] = 'T',
    ['u'] = 'U', ['v'] = 'V', ['w'] = 'w', ['x'] = 'X', ['y'] = 'Y',
    ['z'] = 'Z',

    ['`'] = '~', ['1'] = '!', ['2'] = '@', ['3'] = '#', ['4'] = '$',
    ['5'] = '%', ['6'] = '^', ['7'] = '&', ['8'] = '*', ['9'] = '(',
    ['0'] = ')', ['-'] = '_', ['='] = '+',

    ['['] = '{', [']'] = '}', [';'] = ':', [','] = '<', ['.'] = '>',
    ['/'] = '?', ['\\'] = '|', ['\''] = '\"',
};

char str_keycode_shift(unsigned char c)
{
    if (c > sizeof(str_keycode_shift_map)) return c;
    char result = str_keycode_shift_map[c];
    return result ? result : c;
}
