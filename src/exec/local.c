/* local.c
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sim.h"
#include "game/proxy.h"
#include "render/render.h"


// -----------------------------------------------------------------------------
// local
// -----------------------------------------------------------------------------

int local_run(void)
{
    struct sim *sim = sim_new(0);
    struct sim_pipe *sim_pipe = sim_pipe_new(sim);
    sim_thread(sim);

    struct proxy *proxy = proxy_new();
    struct proxy_pipe *proxy_pipe = proxy_pipe_new(proxy, sim_pipe);

    render_init(proxy);
    render_loop();
    render_close();

    proxy_pipe_close(proxy, proxy_pipe);
    proxy_free(proxy);

    sim_pipe_close(sim_pipe);
    sim_join(sim);
    sim_free(sim);

    return 0;
}
