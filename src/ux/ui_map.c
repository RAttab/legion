/* map.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "ux/ui.h"
#include "game/world.h"
#include "db/res.h"
#include "utils/color.h"
#include "utils/hset.h"

static void ui_map_free(void *);
static void ui_map_update(void *);
static void ui_map_event(void *);
static void ui_map_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// struct
// -----------------------------------------------------------------------------

struct ui_map
{
    struct { struct coord pos; coord_scale scale; } view;
    struct { bool panning, panned; } pan;
    struct { struct rgba select, lanes, sector, area; } s;
};


enum : unit
{
    // Number of pixels per star at scale_base (not
    // map_scale_default). Basically it needs to be tweaked to a number that's
    // big enough to see and click on but not too big that there are overlaps
    // between stars during gen.
    ui_map_star_size = 800,

    // Tweaked in relation to map_star_px so that our default view isn't
    // useless.
    ui_map_scale_default = coord_scale_min << 5,

    // As we zoom out we need to pull more nad more sector data which becomes
    // too expansive. These determine at which zoom threshold do we stop pulling
    // some data and displaying things like the sector or area grid.
    ui_map_thresh_stars = coord_scale_min << 0x8,
    ui_map_thresh_sector_low = coord_scale_min << 0x7,
    ui_map_thresh_sector_high = coord_scale_min << 0xE,
};



// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

void ui_map_alloc(struct ui_view_state *state)
{
    struct ui_map *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_map) {
        .view = {
            .pos = proxy_home(),
            .scale = ui_map_scale_default,
        },

        .s = {
            .select = ui_st.rgba.map.select,
            .lanes = ui_st.rgba.map.lanes,
            .sector = ui_st.rgba.map.sector,
            .area = ui_st.rgba.map.area,
        },
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_map,
        .slots = ui_slot_back,
        .fn = {
            .free = ui_map_free,
            .update_frame = ui_map_update,
            .event = ui_map_event,
            .render = ui_map_render,
        },
    };
}

static void ui_map_free(void *state)
{
    struct ui_map *ui = state;
    (void) ui;
}

void ui_map_show(struct coord coord)
{
    struct ui_map *ui = ui_state(ui_view_map);

    if (!coord_is_nil(coord)) ui->view.pos = coord;
    if (coord_is_nil(ui->view.pos)) ui->view.pos = proxy_home();
    ui->view.scale = ui_map_scale_default;

    ui_show(ui_view_map);
}

void ui_map_goto(struct coord coord)
{
    if (ui_slot(ui_slot_back) == ui_view_map)
        ui_map_show(coord);
}

static void ui_map_update(void *state)
{
    struct ui_map *ui = state;
    if (coord_is_nil(ui->view.pos)) ui->view.pos = proxy_home();
}

coord_scale ui_map_scale(void)
{
    struct ui_map *ui = ui_state(ui_view_map);
    return ui->view.scale;
}


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

static struct coord ui_map_to_coord(struct ui_map *ui, struct pos p)
{
    struct rect area = engine_area();
    assert(area.x == 0 && area.y == 0);
    return make_coord(
            coord_scale_mult(ui->view.scale, p.x - area.w/2) + ui->view.pos.x,
            coord_scale_mult(ui->view.scale, p.y - area.h/2) + ui->view.pos.y);
}

static struct coord_rect ui_map_to_coord_rect(struct ui_map *ui, struct rect r)
{
    return make_coord_rect(
            ui_map_to_coord(ui, rect_pos(r)),
            ui_map_to_coord(ui, make_pos(r.x + r.w, r.y + r.h)));
}

struct coord ui_map_coord(void)
{
    struct ui_map *ui = ui_state(ui_view_map);
    return ui_map_to_coord(ui, ev_mouse_pos());
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ui_map_event(void *state)
{
    struct ui_map *ui = state;

    for (auto ev = ev_load(); ev; ev = nullptr)
        ui->view.pos = proxy_home();

    for (auto ev = ev_next_scroll(nullptr); ev; ev = ev_next_scroll(ev))
        ui->view.scale = coord_scale_inc(ui->view.scale, -ev->dy);

    for (auto ev = ev_mouse(); ev; ev = nullptr) {
        if (!ui->pan.panning) continue;
        if (!ev_button_down(ev_button_left)) {
            ui->pan.panning = ui->pan.panned = false;
            continue;
        }

        unit dx = coord_scale_mult(ui->view.scale, -ev->d.x);
        ui->view.pos.x = legion_bound(ui->view.pos.x + dx, 0, UINT32_MAX);

        unit dy = coord_scale_mult(ui->view.scale, -ev->d.y);
        ui->view.pos.y = legion_bound(ui->view.pos.y + dy, 0, UINT32_MAX);

        ui->pan.panned = true;
    }

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        ev_consume_button(ev);

        if (ev->state == ev_state_down) ui->pan.panning = true;
        if (ev->state != ev_state_up) continue;

        ui->pan.panning = false;
        if (ui->pan.panned) { ui->pan.panned = false; continue; }

        uint32_t d = ui_map_star_size / 2;
        struct coord cursor = ui_map_to_coord(ui, ev_mouse_pos());
        struct coord_rect rect = make_coord_rect(
                make_coord(cursor.x - d, cursor.y - d),
                make_coord(cursor.x + d, cursor.y + d));

        const struct star *star = proxy_star_in(rect);
        if (star) ui_star_show(star->coord);
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static struct pos ui_map_to_pos(struct coord p)
{
    return make_pos(p.x, p.y);
}

static struct rect ui_map_to_rect(struct coord_rect r)
{
    return make_rect(r.top.x, r.top.y, r.bot.x - r.top.x, r.bot.y - r.top.y);
}

static void ui_map_render_areas(
        struct ui_map *ui, const render_layer l, struct coord_rect area)
{
    struct coord start = coord_area(area.top);
    struct coord end = coord_area(area.bot);
    struct rect render_area = ui_map_to_rect(area);

    for (uint32_t x = start.x; x < end.x; x += coord_area_inc) {
        struct line line = { make_pos(x, start.y), make_pos(x, end.y) };
        render_line_a(l, ui->s.area, line, render_area);
    }

    for (uint32_t y = start.y; y < end.y; y += coord_area_inc) {
        struct line line = { make_pos(start.x, y), make_pos(end.x, y) };
        render_line_a(l, ui->s.area, line, render_area);
    }
}

static void ui_map_render_sectors(
        struct ui_map *ui, const render_layer l, struct coord_rect area)
{
    struct coord start = coord_sector(area.top);
    struct coord end = coord_sector(area.bot);
    struct rect render_area = ui_map_to_rect(area);
    struct rgba rgba = ui->s.sector;

    for (uint32_t x = start.x; x < end.x; x += coord_sector_size) {
        struct line line = { make_pos(x, area.top.y), make_pos(x, area.bot.y) };
        render_line_a(l, ui->s.sector, line, render_area);
    }

    for (uint32_t y = start.y; y < end.y; y += coord_sector_size) {
        struct line line = { make_pos(area.top.x, y), make_pos(area.bot.x, y) };
        render_line_a(l, ui->s.sector, line, render_area);
    }

    for (struct coord it = coord_rect_next_sector(area, coord_nil());
         !coord_is_nil(it); it = coord_rect_next_sector(area, it))
    {
        if (!proxy_active_sector(it)) continue;

        rgba.a = 0xFF;
        if (ui->view.scale < ui_map_thresh_stars)
            rgba.a = rgba.a * u64_log2(ui->view.scale) / u64_log2(ui_map_thresh_sector_low);

        struct rect rect = {
            .x = it.x, .y = it.y,
            .w = coord_sector_size,
            .h = coord_sector_size,
        };
        render_rect_fill_a(l, rgba, rect, render_area);
    }
}

static void ui_map_render_lanes(
        struct ui_map *ui,
        const render_layer l,
        struct rect render_area,
        struct coord star)
{
    const struct hset *lanes = proxy_lanes_for(star);
    if (!lanes || !lanes->len) return;

    struct line line = { .a = ui_map_to_pos(star) };

    for (hset_it it = hset_next(lanes, NULL); it; it = hset_next(lanes, it)) {
        line.b = ui_map_to_pos(coord_from_u64(*it));
        render_line_a(l, ui->s.lanes, line, render_area);
    }
}

static void ui_map_render_stars(
        struct ui_map *ui, const render_layer l, struct coord_rect area)
{
    struct rect render_area = ui_map_to_rect(area);
    struct pos cursor = ui_map_to_pos(ui_map_to_coord(ui, ev_mouse_pos()));

    struct coord_rect area_it = area;
    area_it.top.x -= ui_map_star_size/2; area_it.top.y -= ui_map_star_size/2;
    area_it.bot.x += ui_map_star_size/2; area_it.bot.y += ui_map_star_size/2;
    struct proxy_render_it it = proxy_render_it(area_it);

    const struct star *star = NULL;
    while ((star = proxy_render_next(&it))) {
        struct rect rect = {
            .x = star->coord.x - ui_map_star_size / 2,
            .y = star->coord.y - ui_map_star_size / 2,
            .h = ui_map_star_size, .w = ui_map_star_size,
        };

        struct rgba rgba = hsv_to_rgb((struct hsv) {
                    .h = ((double) star->hue) / 360,
                    .s = 1.0 - (((double) star->energy) / UINT16_MAX),
                    .v = rect_contains(rect, cursor) ? 0.8 : 0.5,
                });

        render_star(l + 0, rgba, rect, render_area);

        if (!proxy_active_star(star->coord)) continue;

        ui_map_render_lanes(ui, l + 1, render_area, star->coord);


        const uint32_t w = rect.w / 4;
        struct triangle tri = {
            .a = { rect.x + rect.w/2 - w, rect.y + rect.h/2 - 1*rect.h/2 },
            .b = { rect.x + rect.w/2 + 0, rect.y + rect.h/2 - 15*rect.h/32 },
            .c = { rect.x + rect.w/2 + 0, rect.y + rect.h/2 - 1*rect.h/4 },
        };
        render_triangle_a(l + 1, ui->s.select, tri, render_area);

        tri.a.x = rect.x + rect.w/2 + w;
        render_triangle_a(l + 1, ui->s.select, tri, render_area);
    }
}

static void ui_map_render(void *state, struct ui_layout *)
{
    struct ui_map *ui = state;

    struct coord_rect area = ui_map_to_coord_rect(ui, engine_area());
    const render_layer layer = render_layer_push(3);

    if (ui->view.scale >= ui_map_thresh_sector_low && ui->view.scale < ui_map_thresh_sector_high)
        ui_map_render_sectors(ui, layer + 0, area);

    if (ui->view.scale < ui_map_thresh_stars)
        ui_map_render_stars(ui, layer + 1, area);
    else ui_map_render_areas(ui, layer + 0, area);

    render_layer_pop();
}
