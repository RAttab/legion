/* str.c
   Rémi Attab (remi.attab@gmail.com), 09 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "str.h"


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

bool str_starts_with(const char *str, const char *prefix)
{
    for (; *str && *prefix; ++str, ++prefix)
        if (*str != *prefix)
            return false;
    return !*prefix;
}

bool str_ends_with(const char *str, const char *prefix)
{
    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);
    if (str_len < prefix_len) return false;

    for (size_t i = 1; i <= prefix_len; ++i)
        if (str[str_len - i] != prefix[prefix_len - i])
            return false;
    return true;
}

size_t str_find(
        const char *str, size_t str_len,
        const char *match, size_t match_len)
{
    assert(match_len);

    for (uint32_t it = 0; it + match_len <= str_len; ++it) {
        size_t i = 0;
        while (unlikely(i < match_len && str[it + i] == match[i])) ++i;
        if (unlikely(i == match_len)) return it;
    }

    return str_len;
}


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

size_t str_atod(const char *src, size_t len, int64_t *dst)
{
    if (!len) return 0;

    size_t i = 0;

    bool neg = false;
    if (*src == '-') { neg = true; i++; src++; }

    for (*dst = 0; i < len && (*src >= '0' || *src <= '9'); ++i, ++src)
        *dst = *dst * 10 + (*src - '0');

    if (neg) *dst = -(*dst);
    return i;
}

size_t str_atox(const char *src, size_t len, uint64_t *dst)
{
    size_t i = 0;
    if (len >= 2 && src[0] == '0' && src[1] == 'x') {
        src += 2;
        i += 2;
    }

    uint8_t hex = 0;
    for (*dst = 0; i < len && (hex = str_charhex(*src)) != 0xFF; ++i, ++src)
        *dst = (*dst << 4) | hex;

    return i;
}

size_t str_atou(const char *src, size_t len, uint64_t *dst)
{
    if (len > 2 && src[0] == '0' && src[1] == 'x')
        return str_atox(src+2, len-2, dst);

    size_t i = 0;
    for(*dst = 0; i < len && (*src >= '0' || *src <= '9'); ++i, ++src)
        *dst = *dst * 10 + (*src - '0');
    return i;
}

size_t str_scaled(uint64_t val, char *dst, size_t len)
{
    assert(len >= str_scaled_len);
    static const char units[] = "ukMG?";

    size_t unit = 0;
    while (val >= 1000) {
        val /= 1000;
        unit++;
    }

    dst[3] = units[unit];
    dst[2] = '0' + (val % 10);
    dst[1] = '0' + ((val / 10) % 10);
    dst[0] = '0' + ((val / 100) % 10);

    if (dst[0] == '0') {
        dst[0] = ' ';
        if (dst[1] == '0') dst[1] = ' ';
    }

    return str_scaled_len;
}

void str_to_upper_case(char *str, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        if (str_is_lower_case(str[i]))
            str[i] = str[i] - 'a' + 'A';
}

void str_to_lower_case(char *str, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        if (str_is_upper_case(str[i]))
            str[i] = str[i] - 'A' + 'a';
}

size_t str_skip_spaces(const char *str, size_t len)
{
    const char *it = str;
    while (it < str + len) {
        if (!str_is_space(*it)) break;
        it++;
    }
    return it - str;
}


char str_keycode_shift(unsigned char c)
{
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

    if (c > sizeof(str_keycode_shift_map)) return c;
    char result = str_keycode_shift_map[c];
    return result ? result : c;
}

// -----------------------------------------------------------------------------
// rowcol
// -----------------------------------------------------------------------------

struct rowcol rowcol(const char *str, size_t len)
{
    size_t ix = len - 1;
    struct rowcol rc = {0};

    for (; ix < len; --ix, ++rc.col)
        if (unlikely(str[ix] == '\n')) break;

    for (; ix < len; --ix)
        if (unlikely(str[ix] == '\n')) rc.row++;

    return rc;
}

struct rowcol rowcol_add(struct rowcol lhs, struct rowcol rhs)
{
    return (struct rowcol) {
        .row = lhs.row + rhs.row,
        .col = rhs.row ? rhs.col : lhs.col + rhs.col,
    };
}
