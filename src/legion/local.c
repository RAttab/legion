/* local.c
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// local
// -----------------------------------------------------------------------------

bool local_run(const char *file, world_seed seed)
{
    struct sim *sim = sim_new(seed, file);
    struct sim_pipe *sim_pipe = sim_pipe_new(sim);
    sim_fork(sim);

    proxy_init();
    struct proxy_pipe *proxy_pipe = proxy_pipe_new(sim_pipe);

    engine_init();
    engine_loop();
    engine_close();

    proxy_pipe_close(proxy_pipe);
    proxy_free();

    sim_pipe_close(sim_pipe);
    sim_join(sim);
    sim_free(sim);

    return true;
}
