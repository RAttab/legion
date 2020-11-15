/* coord_test.c
   Rémi Attab (remi.attab@gmail.com), 09 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "coord.h"

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

    fprintf(stderr,
            "project(scale:%lu, sdl:{%dx%d}, exp:{%xx%x}) -> coord:{%xx%x}, sdl:{%dx%d}\n",
            state->scale,
            sdl.x, sdl.y, exp.x, exp.y,
            coord_out.x, coord_out.y, sdl_out.x, sdl_out.y);
    
    assert(coord_out.x == exp.x && coord_out.y == exp.y);
    assert(sdl.x == sdl_out.x && sdl.y == sdl_out.y);
}

void test_project()
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

int main(int argc, char **argv)
{
    (void) argc, (void) argv;
    
    test_project();
}
