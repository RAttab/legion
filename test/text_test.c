/* text_test.c
   Rémi Attab (remi.attab@gmail.com), 21 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/rng.h"
#include "utils/text.h"

void check_str(size_t eol)
{
    char in[s_page_len] = {0};
    char out[sizeof(in)] = {0};
    struct rng rng = rng_make(0);

    for (size_t attempt = 0; attempt < 3; ++attempt) {
        memset(in, 'a', sizeof(in));
        memset(out, 0, sizeof(out));

        size_t n = eol;
        while (n) {
            size_t i = rng_uni(&rng, 0, sizeof(in) + 1);
            if (in[i] == '\n') continue;
            in[i] = '\n';
            --n;
        }

        struct text text = {0};
        text_from_str(&text, in, sizeof(in));

        assert(text.bytes == sizeof(in));
        assert(text.lines == eol+1);

        struct line *line = text.first;
        for (size_t i = 0; i < text.lines; ++i) {
            assert(line);
            line = line->next;
        }
        assert(!line);

        size_t len = text_to_str(&text, out, sizeof(out));
        assert(len == sizeof(in));
        assert(!memcmp(in, out, sizeof(in)));
    }
}

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    check_str(0);
    check_str(1);
    check_str(2);
    check_str(128);

    return 0;
}
