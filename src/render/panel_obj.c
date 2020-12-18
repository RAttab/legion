/* panel_obj.c
   Rémi Attab (remi.attab@gmail.com), 15 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/font.h"
#include "render/panel.h"
#include "render/core.h"
#include "game/item.h"
#include "game/hunk.h"
#include "game/obj.h"
#include "utils/log.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// panel obj
// -----------------------------------------------------------------------------

struct panel_obj_state
{
    id_t id;
    struct coord star;
    struct obj *obj;
    struct layout *layout;
};

enum
{
    p_obj_id = 0,
    p_obj_target,
    p_obj_target_sep,
    p_obj_cargo,
    p_obj_cargo_grid,
    p_obj_cargo_sep,
    p_obj_docks,
    p_obj_docks_list,
    p_obj_docks_sep,
    p_obj_len,
};

enum
{
    p_obj_cargo_cols = 5,
    p_obj_cargo_rows = 2,
    p_obj_docks_max = 5,
};

static const char p_obj_target_str[] = "target:";
static const char p_obj_cargo_str[] = "cargo:";
static const char p_obj_docks_str[] = "docks:";


static struct font *panel_obj_font_big(void) { return font_mono8; }
static struct font *panel_obj_font_small(void) { return font_mono6; }

static void panel_obj_render_id(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_obj_id);
    char str[id_str_len];
    id_str(state->id, sizeof(str), str);
    font_render(layout->font, renderer, str, sizeof(str), layout_entry_pos(layout));
}

static void panel_obj_render_target(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_obj_target);
    font_render(
            layout->font, renderer,
            p_obj_target_str, sizeof(p_obj_target_str),
            layout_entry_pos(layout));

    char str[id_str_len];
    id_str(state->obj->target, sizeof(str), str);
    font_render(layout->font, renderer, str, sizeof(str),
            layout_entry_index_pos(layout, 1, sizeof(p_obj_target)));
}

static void panel_obj_render_cargo(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_obj_cargo);
    font_render(
            layout->font, renderer,
            p_obj_cargo_str, sizeof(p_obj_cargo_str),
            layout_entry_pos(layout));


    size_t len = state->obj->cargos;
    assert(len < layout->rows * layout->cols);

    for (size_t i = 0; i < len; ++i) {
        cargo_t cargo = obj_cargo(state->obj)[i];
        SDL_Rect rect = layout_entry_index(layout, i / layout->cols, i % layout->cols);

        { // border
            uint8_t gray = 0x55;
            sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, 0xAA));
            sdl_err(SDL_RenderFillRect(renderer, &rect));
        }

        { // background
            uint8_t gray = 0x33;
            sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, 0xAA));
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = rect.x + 1, .y = rect.y + 1,
                                .w = rect.w - 2, .h = rect.h - 2 }));
        }

        if (cargo) {
            // todo: draw icon and count.
        }
    }
}

static void panel_obj_render_docks(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_obj_docks);
    font_render(
            layout->font, renderer,
            p_obj_docks_str, sizeof(p_obj_docks_str),
            layout_entry_pos(layout));


    size_t len = state->obj->docks;
    assert(len < layout->rows);

    for (size_t row = 0; row < len; ++row) {
        id_t dock = obj_docks(state->obj)[row];
        SDL_Point pos = layout_entry_index_pos(layout, row, 0);

        char prefix[3] = {'0' + row, ':', ' '};
        font_render(layout->font, renderer, prefix, sizeof(prefix), pos);

        if (dock) {
            char str[id_str_len];
            id_str(dock, sizeof(str), str);
            font_render(layout->font, renderer, str, sizeof(str),
                    layout_entry_index_pos(layout, row, sizeof(prefix)));
        }
        else {
            char empty[] = "empty";
            const uint8_t gray = 0x66;
            sdl_err(SDL_SetTextureColorMod(layout->font->tex, gray, gray, gray));
            font_render(layout->font, renderer, empty, sizeof(empty),
                    layout_entry_index_pos(layout, row, sizeof(prefix)));
            font_reset(layout->font);
        }
    }
}


static void panel_obj_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    (void) rect;

    struct panel_obj_state *state = state_;
    panel_obj_render_id(state, renderer);
    panel_obj_render_target(state, renderer);
    panel_obj_render_cargo(state, renderer);
    panel_obj_render_docks(state, renderer);
}

static void panel_obj_update(struct panel_obj_state *state)
{
    assert(state->id && !coord_null(state->star));

    struct hunk *hunk = sector_hunk(core.state.sector, state->star);
    state->obj = hunk_obj(hunk, state->id);

    assert(state->obj);
}

static bool panel_obj_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_obj_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {

        case EV_OBJ_SELECT: {
            state->id = (uint64_t) event->user.data1;
            state->star = id_to_coord((uint64_t) event->user.data2);
            panel_obj_update(state);
            panel_show(panel);
            break;
        }

        case EV_OBJ_UPDATE: {
            panel_obj_update(state);
            panel_invalidate(panel);
            break;
        }

        case EV_OBJ_CLEAR: {
            state->id = 0;
            state->star = (struct coord) {0};
            state->obj = NULL;
            panel_hide(panel);
            break;
        }

        default: { return false; }
        }
    }

    if (panel->hidden) return false;
    return false;
}


static void panel_obj_free(void *state_)
{
    struct panel_obj_state *state = state_;
    layout_free(state->layout);
    free(state);
};

struct panel *panel_obj_new(void)
{
    struct font *font_b = panel_obj_font_big();
    struct font *font_s = panel_obj_font_small();
    size_t menu_h = panel_menu_height();

    struct layout *layout = layout_alloc(p_obj_len,
            core.rect.w, core.rect.h - menu_h - panel_total_padding);

    layout_text(layout, p_obj_id, font_b, 2+6, 1);

    layout_text(layout, p_obj_target, font_s, sizeof(p_obj_target_str) + id_str_len, 1);
    layout_sep(layout, p_obj_target_sep);

    layout_text(layout, p_obj_cargo, font_s, sizeof(p_obj_cargo_str), 1);
    layout_grid(layout, p_obj_cargo_grid, p_obj_cargo_cols, p_obj_cargo_rows, 25);
    layout_sep(layout, p_obj_cargo_sep);

    layout_text(layout, p_obj_docks, font_s, sizeof(p_obj_docks_str), 1);
    layout_text(layout, p_obj_docks_list, font_s, 3 + id_str_len, p_obj_docks_max);
    layout_sep(layout, p_obj_docks_sep);

    int outer_w = 0, outer_h = 0;
    panel_add_borders(layout->bbox.w, layout->bbox.h, &outer_w, &outer_h);

    SDL_Point rel = { .x = panel_padding, .y = panel_padding };
    SDL_Point abs = {
        .x = core.rect.w - panel_star_width() - outer_w + panel_padding,
        .y = menu_h + panel_padding
    };
    layout_finish(layout, abs, rel);

    struct panel_obj_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = abs.x, .y = abs.y, .w = outer_w, .h = outer_h });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_obj_render;
    panel->events = panel_obj_events;
    panel->free = panel_obj_free;
    return panel;
}
