/* proxy_test.c
   Rémi Attab (remi.attab@gmail.com), 09 Nov 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm.h"
#include "game.h"
#include "engine.h"

#include "utils/vec.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

void check_basics(void)
{
    enum { seed = 123 };

    struct sim *sim = sim_new(123, "./proxy.save");
    struct sim_pipe *sim_pipe = sim_pipe_new(sim, 0);

    proxy_init();
    struct proxy_pipe *proxy_pipe = proxy_pipe_new(sim_pipe);

    sim_step(sim);
    assert(proxy_update());

    {
        const char eval[] = "(mod boot)";
        struct lisp_ret ret = proxy_eval(eval, sizeof(eval));
        assert(ret.ok);

        proxy_io(io_mod, make_im_id(item_brain, 1), &ret.value, 1);
    }

    proxy_set_speed(speed_fast);

    struct coord home = proxy_home();
    assert(!coord_is_nil(home));

    for (size_t i = 0; i < 1000; ++i) {
        assert(proxy_seed() == seed);
        assert(coord_eq(proxy_home(), home));

        assert(proxy_tech());
        assert(proxy_mods()->len > 0);
        assert(proxy_chunks()->len == 1);
        assert(proxy_lanes()->len == 0);

        {
            struct vec64 *atoms = atoms_list(proxy_atoms());
            assert(atoms->len > 0);
            vec64_free(atoms);
        }

        {
            struct chunk *chunk = proxy_chunk(home);
            assert(chunk);
            assert(coord_eq(chunk_star(chunk)->coord, home));
        }

        sim_step(sim);
        while (!proxy_update());
    }

    proxy_pipe_close(proxy_pipe);
    proxy_free();

    sim_pipe_close(sim_pipe);
    sim_free(sim);
}


int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    engine_populate_tests();
    check_basics();

    return 0;
}
