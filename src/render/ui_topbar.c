/* topbar.c
   Rémi Attab (remi.attab@gmail.com), 13 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/gui.h"
#include "ui/ui.h"


// -----------------------------------------------------------------------------
// topbar
// -----------------------------------------------------------------------------

struct ui_topbar
{
    struct panel *panel;
    struct button *mods;
    struct label *coord;
    struct button *close;
};

enum
{
    topbar_ticks_len = 8,
    topbar_coord_len = topbar_ticks_len+1 + coord_str_len+1 + scale_str_len+1,
};

struct ui_topbar *ui_topbar_new(void)
{
    struct font *font = font_mono6;
    struct ui_topbar *topbar = calloc(1, sizeof(*topbar));
    *topbar = (struct ui_topbar) {
        .panel = panel_slim(make_pos(0, 0), make_dim(core.rect.w, 12)),
        .mods = button_const(font, "mods"),
        .coord = label_var(font, topbar_coord_len),
        .close = button_const(font, "x"),
    };

    return topbar;
}

enum ui_ret ui_topbar_event(struct ui_topbar *topbar, SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;
    if ((ret = panel_event(topbar->panel, ev))) return ret;


    if ((ret = button_event(topbar->mods, ev))) {
        // \todo show mods panel.
        return ret;
    }

    if ((ret = button_event(topbar->close, ev))) {
        sdl_err(SDL_PushEvent(&(SDL_Event) { .type = SDL_QUIT }));
        return ret;
    }

    return ui_nil;
}

static void topbar_render_coord(
        struct ui_topbar *topbar, struct layout *layout, SDL_Renderer *renderer)
{
    char buffer[topbar_coord_len] = {0};

    char *it = buffer;
    const char *end = it + sizeof(buffer);

    it += str_utoa(core.state.time, it, end - it);
    *it = ' '; it++;

    struct coord coord = map_project_coord(core.ui.map, core.cursor.point);
    it += coord_str(coord, it, end - it);
    *it = ' '; it++;

    scale_t scale = map_scale(core.ui.map);
    it += scale_str(scale, it, end - it);

    label_set(topbar->coord, buffer, sizeof(buffer));
    label_render(topbar->coord, layout, renderer);
}

void ui_topbar_render(struct ui_topbar *topbar, SDL_Renderer *renderer)
{
    struct layout layout = panel_render(topbar->panel, renderer);

    button_render(topbar->mods, &layout, renderer);

    layout_mid(&layout, &topbar->coord->w);
    topbar_render_coord(topbar, &layout, renderer);

    layout_right(&layout, &topbar->close->w);
    button_render(topbar->close, &layout, renderer);
}
