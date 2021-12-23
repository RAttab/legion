/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sys.h"
#include "render/render.h"

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    sys_populate();

    if (argc == 1) {
        render_init();
        render_run();
        render_close();
        return 0;
    }

    if (argc == 2) {
        if (!strcmp(argv[1], "--viz")) return viz_run(argc, argv);
        if (!strcmp(argv[1], "--stats")) return stats_run(argc, argv);
    }

    assert(false);
    return 1;
}
