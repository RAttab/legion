/* local.c
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// local
// -----------------------------------------------------------------------------

bool local_run(const struct args *args)
{
    threads_init(threads_profile_local);
    if (args->metrics) metrics_open(args->metrics);

    struct sim *sim = sim_new(args->seed, args->save);
    struct sim_pipe *sim_pipe = sim_pipe_new(sim, sys_sec / (engine_frame_rate * 2));
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

    if (args->metrics) metrics_close();
    return true;
}
