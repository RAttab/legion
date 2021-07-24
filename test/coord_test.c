/* coord_test.c
   Rémi Attab (remi.attab@gmail.com), 09 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/coord.h"

#include <assert.h>

struct state
{
    SDL_Rect rect;
    struct coord center;
    scale_t scale;
};

void
check_project(const struct state *state, SDL_Point sdl, struct coord exp)
{
    struct coord coord_out = project_coord(state->rect, state->center, state->scale, sdl);
    SDL_Point sdl_out = project_sdl(state->rect, state->center, state->scale, coord_out);

    if (false) {
        dbg("project(scale:%lu, sdl:{%dx%d}, exp:{%xx%x}) -> coord:{%xx%x}, sdl:{%dx%d}\n",
                state->scale,
                sdl.x, sdl.y, exp.x, exp.y,
                coord_out.x, coord_out.y, sdl_out.x, sdl_out.y);
    }

    assert(coord_out.x == exp.x && coord_out.y == exp.y);
    assert(sdl.x == sdl_out.x && sdl.y == sdl_out.y);
}

void test_project(void)
{
    struct state state = {
        .rect = (SDL_Rect) { .x = 0, .y = 0, .w = 1000, .h = 1000 },
        .center = (struct coord) { .x = 500, .y = 500 },
        .scale = scale_init(),
    };

    check_project(&state,
            (SDL_Point) { .x = 500, .y = 500 },
            (struct coord) { .x = 500, .y = 500 });
    check_project(&state,
            (SDL_Point) { .x = 0, .y = 0 },
            (struct coord) { .x = 0, .y = 0 });

    state.scale = scale_base * 2;
    check_project(&state,
            (SDL_Point) { .x = 500, .y = 500 },
            (struct coord) { .x = 500, .y = 500 });
    check_project(&state,
            (SDL_Point) { .x = 400, .y = 400 },
            (struct coord) { .x = 300, .y = 300 });
    check_project(&state,
            (SDL_Point) { .x = 250, .y = 250 },
            (struct coord) { .x = 0, .y = 0 });

    state.scale = scale_base / 2;
    check_project(&state,
            (SDL_Point) { .x = 500, .y = 500 },
            (struct coord) { .x = 500, .y = 500 });
    check_project(&state,
            (SDL_Point) { .x = 400, .y = 400 },
            (struct coord) { .x = 450, .y = 450 });
    check_project(&state,
            (SDL_Point) { .x = 0, .y = 0 },
            (struct coord) { .x = 250, .y = 250 });
}

void check_rect_next_sector(size_t exp, struct rect rect)
{
    size_t n = 0;

    struct coord prev = coord_nil();
    struct coord it = rect_next_sector(rect, prev);

    for (; !coord_is_nil(it); it = rect_next_sector(rect, it)) {
        n++;

        if (coord_is_nil(prev)) assert(coord_eq(it, rect.top));
        else {
            if (prev.x < it.x) assert(it.x == prev.x + coord_sector_size);
            else {
                assert(it.x == rect.top.x);
                assert(it.y == prev.y + coord_sector_size);
            }
        }

        prev = it;
    }
    assert(n == exp);
}

void test_rect_next_sector(void)
{
    check_rect_next_sector(9, (struct rect) {
                .top = make_coord(1 << coord_sector_bits, 1 << coord_sector_bits),
                .bot = make_coord(3 << coord_sector_bits, 3 << coord_sector_bits),
            });
}

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    test_project();
    test_rect_next_sector();

    return 0;
}
