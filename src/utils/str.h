/* str.h
   Rémi Attab (remi.attab@gmail.com), 09 Dec 2020
   FreeBSD-style copyright and disclaimer apply

   \todo need to write meself a proper string library
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

bool str_starts_with(const char *str, const char *prefix);
bool str_ends_with(const char *str, const char *prefix);
size_t str_find(const char *str, size_t len, const char *match, size_t match_len);

size_t str_utoa(uint64_t val, char *dst, size_t len);
size_t str_utox(uint64_t val, char *dst, size_t len);

size_t str_atod(const char *src, size_t len, int64_t *dst);
size_t str_atou(const char *src, size_t len, uint64_t *dst);
size_t str_atox(const char *src, size_t len, uint64_t *dst);

enum { str_scaled_len = 4 };
size_t str_scaled(uint64_t val, char *dst, size_t len);

inline char str_hexchar(uint8_t val)
{
    val &= 0xF;
    return val < 0xA ? '0' + val : 'A' + (val - 0xA);
}

inline uint8_t str_charhex(char val)
{
    if (val >= '0' && val <= '9') return val - '0';
    if (val >= 'A' && val <= 'F') return 10 + (val - 'A');
    if (val >= 'a' && val <= 'f') return 10 + (val - 'a');
    return 0xFF;
}

inline bool str_is_number(char c)
{
    switch (c) {
    case '-': case 'x':
    case '0'...'9':
    case 'a'...'f':
    case 'A'...'F':
        return true;

    default: return false;
    }
}

inline bool str_is_vowel(char c)
{
    switch (c) {
    case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
    case 'A': case 'E': case 'I': case 'O': case 'U': case 'Y':
        return true;
    default: return false;
    }
}

inline bool str_is_upper_case(char c) { return c >= 'A' && c <= 'Z'; }
inline bool str_is_lower_case(char c) { return c >= 'a' && c <= 'z'; }

inline char str_upper_case(char c) { return str_is_lower_case(c) ? c - 'a' + 'A' : c; }
inline char str_lower_case(char c) { return str_is_upper_case(c) ? c - 'A' + 'a' : c; }

inline void str_to_upper_case(char *str, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        str[i] = str_upper_case(str[i]);
}
inline void str_to_lower_case(char *str, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        str[i] = str_lower_case(str[i]);
}

inline bool str_is_space(char c) { return c <= 0x20; }
size_t str_skip_spaces(const char *str, size_t len);

char str_keycode_shift(unsigned char c);

// -----------------------------------------------------------------------------
// rowcol
// -----------------------------------------------------------------------------

struct rowcol { uint16_t row, col; };

struct rowcol rowcol(const char *str, size_t len);
struct rowcol rowcol_add(struct rowcol, struct rowcol);

inline int rowcol_cmp(struct rowcol lhs, struct rowcol rhs)
{
    return lhs.row == rhs.row ? (
            lhs.col < rhs.col ? -1 :
            lhs.col > rhs.col ? +1 : 0) : (
                    lhs.row < rhs.row ? -1 :
                    lhs.row > rhs.row ? +1 : 0);
}

inline struct rowcol make_rowcol(uint16_t row, int16_t col)
{
    return (struct rowcol) { .row = row, .col = col };
}
