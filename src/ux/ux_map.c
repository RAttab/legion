/* map.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_map_free(void *);
static void ux_map_update(void *);
static void ux_map_event(void *);
static void ux_map_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// struct
// -----------------------------------------------------------------------------

struct ux_map
{
    struct { struct coord pos; coord_scale scale; } view;
    struct { bool panning, panned; } pan;
    struct { struct rgba select, lanes, sector, area; } s;
};


// Number of pixels per star at scale_base (not map_scale_default). Basically it
// needs to be tweaked to a number that's big enough to see and click on but not
// too big that there are overlaps between stars during gen.
constexpr unit ux_map_star_size = 800;

// Tweaked in relation to map_star_px so that our default view isn't useless.
constexpr unit ux_map_scale_default = coord_scale_min << 5;

// As we zoom out we need to pull more nad more sector data which becomes too
// expansive. These determine at which zoom threshold do we stop pulling some
// data and displaying things like the sector or area grid.
constexpr unit ux_map_thresh_stars = coord_scale_min << 0x8;
constexpr unit ux_map_thresh_sector_low = coord_scale_min << 0x7;
constexpr unit ux_map_thresh_sector_high = coord_scale_min << 0xE;



// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

void ux_map_alloc(struct ux_view_state *state)
{
    struct ux_map *ux = calloc(1, sizeof(*ux));
    *ux = (struct ux_map) {
        .view = {
            .pos = proxy_home(),
            .scale = ux_map_scale_default,
        },

        .s = {
            .select = ui_st.rgba.map.select,
            .lanes = ui_st.rgba.map.lanes,
            .sector = ui_st.rgba.map.sector,
            .area = ui_st.rgba.map.area,
        },
    };

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_map,
        .slots = ux_slot_back,
        .fn = {
            .free = ux_map_free,
            .update_frame = ux_map_update,
            .event = ux_map_event,
            .render = ux_map_render,
        },
    };
}

static void ux_map_free(void *state)
{
    struct ux_map *ux = state;
    (void) ux;
}

void ux_map_show(struct coord coord)
{
    struct ux_map *ux = ux_state(ux_view_map);

    if (!coord_is_nil(coord)) ux->view.pos = coord;
    if (coord_is_nil(ux->view.pos)) ux->view.pos = proxy_home();
    ux->view.scale = ux_map_scale_default;

    ux_show(ux_view_map);
}

void ux_map_goto(struct coord coord)
{
    if (ux_slot(ux_slot_back) == ux_view_map)
        ux_map_show(coord);
}

static void ux_map_update(void *state)
{
    struct ux_map *ux = state;
    if (coord_is_nil(ux->view.pos)) ux->view.pos = proxy_home();
}

coord_scale ux_map_scale(void)
{
    struct ux_map *ux = ux_state(ux_view_map);
    return ux->view.scale;
}


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

static struct coord ux_map_to_coord(struct ux_map *ux, struct pos p)
{
    struct rect area = engine_area();
    assert(area.x == 0 && area.y == 0);
    return make_coord(
            coord_scale_mult(ux->view.scale, p.x - area.w/2) + ux->view.pos.x,
            coord_scale_mult(ux->view.scale, p.y - area.h/2) + ux->view.pos.y);
}

static struct coord_rect ux_map_to_coord_rect(struct ux_map *ux, struct rect r)
{
    return make_coord_rect(
            ux_map_to_coord(ux, rect_pos(r)),
            ux_map_to_coord(ux, make_pos(r.x + r.w, r.y + r.h)));
}

struct coord ux_map_coord(void)
{
    struct ux_map *ux = ux_state(ux_view_map);
    return ux_map_to_coord(ux, ev_mouse_pos());
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ux_map_event(void *state)
{
    struct ux_map *ux = state;

    for (auto ev = ev_load(); ev; ev = nullptr)
        ux->view.pos = proxy_home();

    for (auto ev = ev_next_scroll(nullptr); ev; ev = ev_next_scroll(ev))
        ux->view.scale = coord_scale_inc(ux->view.scale, -ev->dy);

    for (auto ev = ev_mouse(); ev; ev = nullptr) {
        if (!ux->pan.panning) continue;
        if (!ev_button_down(ev_button_left)) {
            ux->pan.panning = ux->pan.panned = false;
            continue;
        }

        unit dx = coord_scale_mult(ux->view.scale, -ev->d.x);
        ux->view.pos.x = legion_bound(ux->view.pos.x + dx, 0, UINT32_MAX);

        unit dy = coord_scale_mult(ux->view.scale, -ev->d.y);
        ux->view.pos.y = legion_bound(ux->view.pos.y + dy, 0, UINT32_MAX);

        ux->pan.panned = true;
    }

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        ev_consume_button(ev);

        if (ev->state == ev_state_down) ux->pan.panning = true;
        if (ev->state != ev_state_up) continue;

        ux->pan.panning = false;
        if (ux->pan.panned) { ux->pan.panned = false; continue; }

        uint32_t d = ux_map_star_size / 2;
        struct coord cursor = ux_map_to_coord(ux, ev_mouse_pos());
        struct coord_rect rect = make_coord_rect(
                make_coord(cursor.x - d, cursor.y - d),
                make_coord(cursor.x + d, cursor.y + d));

        const struct star *star = proxy_star_in(rect);
        if (star) ux_star_show(star->coord);
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static struct pos ux_map_to_pos(struct coord p)
{
    return make_pos(p.x, p.y);
}

static struct rect ux_map_to_rect(struct coord_rect r)
{
    return make_rect(r.top.x, r.top.y, r.bot.x - r.top.x, r.bot.y - r.top.y);
}

static void ux_map_render_areas(
        struct ux_map *ux, const render_layer l, struct coord_rect area)
{
    struct coord start = coord_area(area.top);
    struct coord end = coord_area(area.bot);
    struct rect render_area = ux_map_to_rect(area);

    for (uint32_t x = start.x; x < end.x; x += coord_area_inc) {
        struct line line = { make_pos(x, start.y), make_pos(x, end.y) };
        render_line_a(l, ux->s.area, line, render_area);
    }

    for (uint32_t y = start.y; y < end.y; y += coord_area_inc) {
        struct line line = { make_pos(start.x, y), make_pos(end.x, y) };
        render_line_a(l, ux->s.area, line, render_area);
    }
}

static void ux_map_render_sectors(
        struct ux_map *ux, const render_layer l, struct coord_rect area)
{
    struct coord start = coord_sector(area.top);
    struct coord end = coord_sector(area.bot);
    struct rect render_area = ux_map_to_rect(area);
    struct rgba rgba = ux->s.sector;

    for (uint32_t x = start.x; x < end.x; x += coord_sector_size) {
        struct line line = { make_pos(x, area.top.y), make_pos(x, area.bot.y) };
        render_line_a(l, ux->s.sector, line, render_area);
    }

    for (uint32_t y = start.y; y < end.y; y += coord_sector_size) {
        struct line line = { make_pos(area.top.x, y), make_pos(area.bot.x, y) };
        render_line_a(l, ux->s.sector, line, render_area);
    }

    for (struct coord it = coord_rect_next_sector(area, coord_nil());
         !coord_is_nil(it); it = coord_rect_next_sector(area, it))
    {
        if (!proxy_active_sector(it)) continue;

        rgba.a = 0xFF;
        if (ux->view.scale < ux_map_thresh_stars)
            rgba.a = rgba.a * u64_log2(ux->view.scale) / u64_log2(ux_map_thresh_sector_low);

        struct rect rect = {
            .x = it.x, .y = it.y,
            .w = coord_sector_size,
            .h = coord_sector_size,
        };
        render_rect_fill_a(l, rgba, rect, render_area);
    }
}

static void ux_map_render_lanes(
        struct ux_map *ux,
        const render_layer l,
        struct rect render_area,
        struct coord star)
{
    const struct hset *lanes = proxy_lanes_for(star);
    if (!lanes || !lanes->len) return;

    struct line line = { .a = ux_map_to_pos(star) };

    for (hset_it it = hset_next(lanes, NULL); it; it = hset_next(lanes, it)) {
        line.b = ux_map_to_pos(coord_from_u64(*it));
        render_line_a(l, ux->s.lanes, line, render_area);
    }
}

static void ux_map_render_stars(
        struct ux_map *ux, const render_layer l, struct coord_rect area)
{
    struct rect render_area = ux_map_to_rect(area);

    struct pos cursor = ux_cursor_panel() ? make_pos(unit_min, unit_min) :
        ux_map_to_pos(ux_map_to_coord(ux, ev_mouse_pos()));

    struct coord_rect area_it = area;
    area_it.top.x -= ux_map_star_size/2; area_it.top.y -= ux_map_star_size/2;
    area_it.bot.x += ux_map_star_size/2; area_it.bot.y += ux_map_star_size/2;
    struct proxy_render_it it = proxy_render_it(area_it);

    const struct star *star = NULL;
    while ((star = proxy_render_next(&it))) {
        struct rect rect = {
            .x = star->coord.x - ux_map_star_size / 2,
            .y = star->coord.y - ux_map_star_size / 2,
            .h = ux_map_star_size, .w = ux_map_star_size,
        };

        struct rgba rgba = hsv_to_rgb((struct hsv) {
                    .h = ((double) star->hue) / 360,
                    .s = 1.0 - (((double) star->energy) / UINT16_MAX),
                    .v = rect_contains(rect, cursor) ? 0.8 : 0.5,
                });

        render_star(l + 0, rgba, rect, render_area);

        if (!proxy_active_star(star->coord)) continue;

        ux_map_render_lanes(ux, l + 1, render_area, star->coord);


        const uint32_t w = rect.w / 4;
        struct triangle tri = {
            .a = { rect.x + rect.w/2 - w, rect.y + rect.h/2 - 1*rect.h/2 },
            .b = { rect.x + rect.w/2 + 0, rect.y + rect.h/2 - 15*rect.h/32 },
            .c = { rect.x + rect.w/2 + 0, rect.y + rect.h/2 - 1*rect.h/4 },
        };
        render_triangle_a(l + 1, ux->s.select, tri, render_area);

        tri.a.x = rect.x + rect.w/2 + w;
        render_triangle_a(l + 1, ux->s.select, tri, render_area);
    }
}

static void ux_map_render(void *state, struct ui_layout *)
{
    struct ux_map *ux = state;

    struct coord_rect area = ux_map_to_coord_rect(ux, engine_area());
    const render_layer layer = render_layer_push(3);

    if (ux->view.scale >= ux_map_thresh_sector_low && ux->view.scale < ux_map_thresh_sector_high)
        ux_map_render_sectors(ux, layer + 0, area);

    if (ux->view.scale < ux_map_thresh_stars)
        ux_map_render_stars(ux, layer + 1, area);
    else ux_map_render_areas(ux, layer + 0, area);

    render_layer_pop();
}
