/* topbar.c
   Rémi Attab (remi.attab@gmail.com), 13 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


static void ui_topbar_free(void *);
static void ui_topbar_update(void *);
static void ui_topbar_event(void *);
static void ui_topbar_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// topbar
// -----------------------------------------------------------------------------

struct ui_topbar
{
    struct ui_panel *panel;
    struct ui_button save, load;
    struct ui_button pause, slow, fast, faster, fastest;
    struct ui_button home, stars, tapes, mods, log;
    struct ui_label coord;
    struct ui_button man;
    struct ui_button close;
};

enum : uint64_t
{
    topbar_ticks_len = 8,
    topbar_coord_len = topbar_ticks_len+1 + coord_str_len+1 + coord_scale_str_len+1,
};

void ui_topbar_alloc(struct ui_view_state *state)
{
    struct ui_topbar *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_topbar) {
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

        .man = ui_button_new(ui_str_c("?")),
        .close = ui_button_new(ui_str_c("x")),
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_topbar,
        .panel = ui->panel,
        .fn = {
            .free = ui_topbar_free,
            .update_frame = ui_topbar_update,
            .event = ui_topbar_event,
            .render = ui_topbar_render,
        },
    };
}

static void ui_topbar_free(void *state) {
    struct ui_topbar *ui = state;

    ui_panel_free(ui->panel);

    ui_button_free(&ui->save);
    ui_button_free(&ui->load);

    ui_button_free(&ui->pause);
    ui_button_free(&ui->slow);
    ui_button_free(&ui->fast);
    ui_button_free(&ui->faster);
    ui_button_free(&ui->fastest);

    ui_button_free(&ui->home);
    ui_button_free(&ui->stars);
    ui_button_free(&ui->tapes);
    ui_button_free(&ui->mods);
    ui_button_free(&ui->log);

    ui_label_free(&ui->coord);

    ui_button_free(&ui->man);
    ui_button_free(&ui->close);

    free(ui);
}

static void ui_topbar_update(void *state)
{
    struct ui_topbar *ui = state;

    ui->pause.disabled = false;
    ui->slow.disabled = false;
    ui->fast.disabled = false;
    ui->faster.disabled = false;
    ui->fastest.disabled = false;

    switch (proxy_speed())
    {
    case speed_pause: { ui->pause.disabled = true; break; }
    case speed_slow: { ui->slow.disabled = true; break; }
    case speed_fast: { ui->fast.disabled = true; break; }
    case speed_faster: { ui->faster.disabled = true; break; }
    case speed_fastest: { ui->fastest.disabled = true; break; }
    default: { assert(false); }
    }
}

static void ui_topbar_event(void *state)
{
    struct ui_topbar *ui = state;

    if (ui_button_event(&ui->save)) proxy_save();
    if (ui_button_event(&ui->load)) proxy_load();

    if (ui_button_event(&ui->pause)) proxy_set_speed(speed_pause);
    if (ui_button_event(&ui->slow)) proxy_set_speed(speed_slow);
    if (ui_button_event(&ui->fast)) proxy_set_speed(speed_fast);
    if (ui_button_event(&ui->faster)) proxy_set_speed(speed_faster);
    if (ui_button_event(&ui->fastest)) proxy_set_speed(speed_fastest);

    if (ui_button_event(&ui->home)) ui_map_show(proxy_home());
    if (ui_button_event(&ui->stars)) ui_toggle(ui_view_stars);
    if (ui_button_event(&ui->tapes)) ui_toggle(ui_view_tapes);
    if (ui_button_event(&ui->mods)) ui_toggle(ui_view_mods);
    if (ui_button_event(&ui->log)) ui_toggle(ui_view_log);

    if (ui_button_event(&ui->man)) ui_toggle(ui_view_man);
    if (ui_button_event(&ui->close)) engine_quit();
}

static void topbar_render_coord(struct ui_topbar *ui, struct ui_layout *layout)
{
    char buffer[topbar_coord_len] = {0};

    char *it = buffer;
    const char *end = it + sizeof(buffer);

    it += str_utoa(proxy_time(), it, topbar_ticks_len);
    *it = ' '; it++; assert(it < end);

    coord_scale scale = 0;
    struct coord coord = {0};
    switch (ui_slot(ui_slot_back)) {
    case ui_view_map: {
        scale = ui_map_scale();
        coord = ui_map_coord();
        break;
    }
    case ui_view_factory: {
        scale = ui_factory_scale();
        coord = ui_factory_coord();
        break;
    }
    default: { assert(false); }
    }

    it += coord_str(coord, it, end - it);
    *it = ' '; it++; assert(it < end);

    it += coord_scale_str(scale, it, end - it);
    assert(it <= end);

    ui_str_setv(&ui->coord.str, buffer, sizeof(buffer));
    ui_label_render(&ui->coord, layout);
}

static void ui_topbar_render(void *state, struct ui_layout *layout)
{
    struct ui_topbar *ui = state;

    ui_button_render(&ui->save, layout);
    ui_button_render(&ui->load, layout);
    ui_layout_sep_x(layout, 10);

    ui_button_render(&ui->pause, layout);
    ui_button_render(&ui->slow, layout);
    ui_button_render(&ui->fast, layout);
    ui_button_render(&ui->faster, layout);
    ui_button_render(&ui->fastest, layout);
    ui_layout_sep_x(layout, 10);

    ui_button_render(&ui->home, layout);
    ui_layout_sep_x(layout, 10);
    ui_button_render(&ui->stars, layout);
    ui_button_render(&ui->tapes, layout);
    ui_button_render(&ui->mods, layout);
    ui_button_render(&ui->log, layout);

    ui_layout_dir(layout, ui_layout_right_left);
    ui_button_render(&ui->close, layout);
    ui_layout_sep_x(layout, 10);
    ui_button_render(&ui->man, layout);
    ui_layout_dir(layout, ui_layout_left_right);

    ui_layout_mid(layout, ui->coord.w.w);
    topbar_render_coord(ui, layout);
}
