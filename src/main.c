/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/core.h"

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    core_init();
    core_run();
    core_close();

    return 0;
}
