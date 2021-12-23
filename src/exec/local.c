/* local.c
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sim.h"
#include "render/render.h"


// -----------------------------------------------------------------------------
// local
// -----------------------------------------------------------------------------

int local_run(void)
{
    struct sim *sim = sim_new(0);
    struct sim_pipe *pipe = sim_pipe_new(sim);
    sim_thread(sim);

    render_init(sim_pipe_out(pipe), sim_pipe_in(pipe));
    render_loop();
    render_close();

    sim_join(sim);
    sim_free(sim);

    return 0;
}
