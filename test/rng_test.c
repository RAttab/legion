/* rng_test.c
   Rémi Attab (remi.attab@gmail.com), 01 May 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/rng.h"
#include "utils/bits.h"

// -----------------------------------------------------------------------------
// histo
// -----------------------------------------------------------------------------

enum { range = 100, iterations = 1000000 };

struct histo
{
    size_t n, max;
    size_t buckets[range];
};

void histo_reset(struct histo *histo) { memset(histo, 0, sizeof(*histo)); }

void histo_add(struct histo *histo, size_t value)
{
    assert(value < range);

    histo->buckets[value]++;
    histo->max = legion_max(histo->max, histo->buckets[value]);
    histo->n++;
}

void histo_dump(struct histo *histo, const char *title)
{
    enum { rows = 20, cols = 100 };

    char line[cols];

    {
        memset(line, '-', cols);
        size_t len = strlen(title);
        fprintf(stdout, "[ %s ] %.*s\n", title, (unsigned) (cols - len), line);
    }

    size_t per = u64_ceil_div(range, rows);
    for (size_t row = 0; row < rows; ++row) {

        size_t sum = 0;
        for (size_t i = 0; i < per; ++i)
            sum += histo->buckets[i + (row * per)];
        size_t value = ((sum / per) * cols) / histo->max;
        assert(value <= cols);

        memset(line, '=', value);
        memset(line + value, ' ', sizeof(line) - value);
        fprintf(stdout, "%02u | %.*s\n", (unsigned) (row * per), cols, line);
    }

    fprintf(stdout, "\n");
}

// -----------------------------------------------------------------------------
// checks
// -----------------------------------------------------------------------------

void check_uni(void)
{
    struct histo histo = {0};
    histo_reset(&histo);

    struct rng rng = rng_make(0);
    for (size_t it = 0; it < iterations; ++it)
        histo_add(&histo, rng_uni(&rng, 0, range));

    histo_dump(&histo, "uni");
}

void check_exp(void)
{
    struct histo histo = {0};
    histo_reset(&histo);

    struct rng rng = rng_make(0);
    for (size_t it = 0; it < iterations; ++it)
        histo_add(&histo, rng_exp(&rng, 0, range));

    histo_dump(&histo, "exp");
}

void check_norm(void)
{
    struct histo histo = {0};
    histo_reset(&histo);

    struct rng rng = rng_make(0);
    for (size_t it = 0; it < iterations; ++it)
        histo_add(&histo, rng_norm(&rng, 0, range));

    histo_dump(&histo, "norm");
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int argc, const char *argv[])
{
    (void) argc, (void) argv;

    check_uni();
    check_exp();
    check_norm();
}
