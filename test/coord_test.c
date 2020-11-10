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
check_project(const struct state *state, SDL_Point ui, struct coord exp)
{
    struct coord coord = project_coord(state->rect, state->center, state->scale, ui);
    SDL_Point ret = project_ui(state->rect, state->center, state->scale, coord);

    fprintf(stderr,
            "project(scale:%d, ui:{%dx%d}, exp:{%ux%u}) -> coord:{%dx%d}, ret:{%ux%u}\n",
            state->scale,
            ui.x, ui.y, exp.x, exp.y,
            coord.x, coord.y, ret.x, ret.y);
    
    assert(coord.x == exp.x && coord.y == exp.y);
    assert(ui.x == ret.x && ui.y == ret.y);
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
