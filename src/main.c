/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/core.h"

// -----------------------------------------------------------------------------
// implementations
// -----------------------------------------------------------------------------

#include "viz.c"
#include "stats.c"

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    if (argc == 1) {
        core_init();
        core_run();
        core_close();
        return 0;
    }

    if (argc == 2) {
        if (!strcmp(argv[1], "--viz")) return viz_run(argc, argv);
        if (!strcmp(argv[1], "--stats")) return stats_run(argc, argv);
    }

    assert(false);
    return 1;
}
