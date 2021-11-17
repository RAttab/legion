/* proxy_test.c
   Rémi Attab (remi.attab@gmail.com), 09 Nov 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "game/sim.h"
#include "game/proxy.h"
#include "game/chunk.h"
#include "game/sector.h"
#include "render/core.h"
#include "utils/vec.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

void check_basics(void)
{
    enum { seed = 123 };

    struct sim *sim = sim_new(123);
    struct proxy *proxy = proxy_new(sim);

    {
        const char eval[] = "(mod boot)";
        struct lisp_ret ret = proxy_eval(proxy, eval, sizeof(eval));
        assert(ret.ok);

        proxy_io(proxy, IO_MOD, make_id(ITEM_BRAIN, 1), &ret.value, 1);
    }

    proxy_set_speed(proxy, speed_fast);

    struct coord home = proxy_home(proxy);
    assert(!coord_is_nil(home));

    for (size_t i = 0; i < 1000; ++i) {
        assert(proxy_seed(proxy) == seed);
        assert(coord_eq(proxy_home(proxy), home));

        assert(proxy_tech(proxy));
        assert(proxy_mods(proxy)->len > 0);
        assert(proxy_chunks(proxy)->len == 1);
        assert(proxy_lanes(proxy)->len == 0);

        {
            struct vec64 *atoms = atoms_list(proxy_atoms(proxy));
            assert(atoms->len > 0);
            free(atoms);
        }

        {
            struct chunk *chunk = proxy_chunk(proxy, home);
            assert(chunk);
            assert(coord_eq(chunk_star(chunk)->coord, home));
        }

        while (!proxy_update(proxy));
    }

    proxy_free(proxy);
    sim_close(sim);
}


int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    core_populate();
    check_basics();

    return 0;
}
