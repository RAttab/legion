/* topbar.c
   Rémi Attab (remi.attab@gmail.com), 13 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_topbar_free(void *);
static void ux_topbar_update(void *);
static void ux_topbar_event(void *);
static void ux_topbar_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// topbar
// -----------------------------------------------------------------------------

struct ux_topbar
{
    struct ui_panel *panel;
    struct ui_button save, load;
    struct ui_button pause, slow, fast, faster, fastest;
    struct ui_button home, stars, tapes, mods, log;
    struct ui_label coord;

    struct ui_button music;
    struct ui_button man;
    struct ui_button close;
};

constexpr size_t topbar_ticks_len = 8;
constexpr size_t topbar_coord_len = topbar_ticks_len+1 + coord_str_len+1 + coord_scale_str_len+1;

void ux_topbar_alloc(struct ux_view_state *state)
{
    struct ux_topbar *ux = calloc(1, sizeof(*ux));
    *ux = (struct ux_topbar) {
        .panel = ui_panel_menu(
                make_dim(ui_layout_inf, ui_st.button.base.height)),

        .save = ui_button_new(ui_str_c("save")),
        .load = ui_button_new(ui_str_c("load")),

        .pause = ui_button_new(ui_str_c("||")),
        .slow = ui_button_new(ui_str_c(">")),
        .fast = ui_button_new(ui_str_c(">>")),
        .faster = ui_button_new(ui_str_c(">>>")),
        .fastest = ui_button_new(ui_str_c(">>|")),

        .home = ui_button_new(ui_str_c("home")),
        .stars = ui_button_new(ui_str_c("stars")),
        .tapes = ui_button_new(ui_str_c("tapes")),
        .mods = ui_button_new(ui_str_c("mods")),
        .log = ui_button_new(ui_str_c("log")),

        .coord = ui_label_new(ui_str_v(topbar_coord_len)),

        .music = ui_button_new(ui_str_c("music")),

        .man = ui_button_new(ui_str_c("?")),
        .close = ui_button_new(ui_str_c("x")),
    };

    ux->music.s.align = ui_align_center;

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_topbar,
        .panel = ux->panel,
        .fn = {
            .free = ux_topbar_free,
            .update_frame = ux_topbar_update,
            .event = ux_topbar_event,
            .render = ux_topbar_render,
        },
    };
}

static void ux_topbar_free(void *state) {
    struct ux_topbar *ux = state;

    ui_panel_free(ux->panel);

    ui_button_free(&ux->save);
    ui_button_free(&ux->load);

    ui_button_free(&ux->pause);
    ui_button_free(&ux->slow);
    ui_button_free(&ux->fast);
    ui_button_free(&ux->faster);
    ui_button_free(&ux->fastest);

    ui_button_free(&ux->home);
    ui_button_free(&ux->stars);
    ui_button_free(&ux->tapes);
    ui_button_free(&ux->mods);
    ui_button_free(&ux->log);

    ui_label_free(&ux->coord);

    ui_button_free(&ux->man);
    ui_button_free(&ux->close);

    free(ux);
}

static void ux_topbar_update(void *state)
{
    struct ux_topbar *ux = state;

    ux->pause.disabled = false;
    ux->slow.disabled = false;
    ux->fast.disabled = false;
    ux->faster.disabled = false;
    ux->fastest.disabled = false;

    switch (proxy_speed())
    {
    case speed_pause: { ux->pause.disabled = true; break; }
    case speed_slow: { ux->slow.disabled = true; break; }
    case speed_fast: { ux->fast.disabled = true; break; }
    case speed_faster: { ux->faster.disabled = true; break; }
    case speed_fastest: { ux->fastest.disabled = true; break; }
    default: { assert(false); }
    }

    ui_str_setc(&ux->music.str, sound_bgm_paused() ? "music" : "mute");
}

static void ux_topbar_event(void *state)
{
    struct ux_topbar *ux = state;

    if (ui_button_event(&ux->save)) proxy_save();
    if (ui_button_event(&ux->load)) proxy_load();

    if (ui_button_event(&ux->pause)) proxy_set_speed(speed_pause);
    if (ui_button_event(&ux->slow)) proxy_set_speed(speed_slow);
    if (ui_button_event(&ux->fast)) proxy_set_speed(speed_fast);
    if (ui_button_event(&ux->faster)) proxy_set_speed(speed_faster);
    if (ui_button_event(&ux->fastest)) proxy_set_speed(speed_fastest);

    if (ui_button_event(&ux->home)) ux_map_show(proxy_home());
    if (ui_button_event(&ux->stars)) ux_toggle(ux_view_stars);
    if (ui_button_event(&ux->tapes)) ux_toggle(ux_view_tapes);
    if (ui_button_event(&ux->mods)) ux_toggle(ux_view_mods);
    if (ui_button_event(&ux->log)) ux_toggle(ux_view_log);

    if (ui_button_event(&ux->music)) sound_bgm_pause(!sound_bgm_paused());

    if (ui_button_event(&ux->man)) ux_toggle(ux_view_man);
    if (ui_button_event(&ux->close)) engine_quit();
}

static void topbar_render_coord(struct ux_topbar *ux, struct ui_layout *layout)
{
    char buffer[topbar_coord_len] = {0};

    char *it = buffer;
    const char *end = it + sizeof(buffer);

    it += str_utoa(proxy_time(), it, topbar_ticks_len);
    *it = ' '; it++; assert(it < end);

    coord_scale scale = 0;
    struct coord coord = {0};
    switch (ux_slot(ux_slot_back)) {
    case ux_view_map: {
        scale = ux_map_scale();
        coord = ux_map_coord();
        break;
    }
    case ux_view_factory: {
        scale = ux_factory_scale();
        coord = ux_factory_coord();
        break;
    }
    default: { assert(false); }
    }

    it += coord_str(coord, it, end - it);
    *it = ' '; it++; assert(it < end);

    it += coord_scale_str(scale, it, end - it);
    assert(it <= end);

    ui_str_setv(&ux->coord.str, buffer, sizeof(buffer));
    ui_label_render(&ux->coord, layout);
}

static void ux_topbar_render(void *state, struct ui_layout *layout)
{
    struct ux_topbar *ux = state;

    ui_button_render(&ux->save, layout);
    ui_button_render(&ux->load, layout);
    ui_layout_sep_x(layout, 10);

    ui_button_render(&ux->pause, layout);
    ui_button_render(&ux->slow, layout);
    ui_button_render(&ux->fast, layout);
    ui_button_render(&ux->faster, layout);
    ui_button_render(&ux->fastest, layout);
    ui_layout_sep_x(layout, 10);

    ui_button_render(&ux->home, layout);
    ui_layout_sep_x(layout, 10);
    ui_button_render(&ux->stars, layout);
    ui_button_render(&ux->tapes, layout);
    ui_button_render(&ux->mods, layout);
    ui_button_render(&ux->log, layout);

    ui_layout_dir(layout, ui_layout_right_left);
    ui_button_render(&ux->close, layout);
    ui_layout_sep_x(layout, 10);
    ui_button_render(&ux->man, layout);
    ui_layout_sep_x(layout, 10);
    ui_button_render(&ux->music, layout);
    ui_layout_dir(layout, ui_layout_left_right);

    ui_layout_mid(layout, ux->coord.w.w);
    topbar_render_coord(ux, layout);
}
