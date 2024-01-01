/* ux_workers.c
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_workers_free(void *);
static void ux_workers_hide(void *);
static void ux_workers_update(void *);
static void ux_workers_event(void *);
static void ux_workers_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

enum
{
    ux_workers_queue = 0,
    ux_workers_work,
    ux_workers_clean,
    ux_workers_fail,
    ux_workers_idle,
};

struct ux_workers
{
    struct coord star;
    world_ts last_t;

    struct ui_panel *panel;
    struct ui_histo histo;
};


void ux_workers_alloc(struct ux_view_state *state)
{
    struct ui_histo_series series[] = {
        [ux_workers_queue] = { 0, "queue", ui_st.rgba.worker.queue, true },
        [ux_workers_work] =  { 1, "work", ui_st.rgba.worker.work, true },
        [ux_workers_clean] = { 1, "clean", ui_st.rgba.worker.clean, true },
        [ux_workers_fail] =  { 1, "fail", ui_st.rgba.worker.fail, true },
        [ux_workers_idle] =  { 1, "idle", ui_st.rgba.worker.idle, true },
    };

    struct ux_workers *ux = calloc(1, sizeof(*ux));
    *ux = (struct ux_workers) {
        .star = coord_nil(),

        .panel = ui_panel_title(
                make_dim(77 * engine_cell().w, ui_layout_inf),
                ui_str_c("workers")),

        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_workers,
        .parent = ux_view_star,
        .slots = ux_slot_right_sub,
        .panel = ux->panel,
        .fn = {
            .free = ux_workers_free,
            .hide = ux_workers_hide,
            .update = ux_workers_update,
            .event = ux_workers_event,
            .render = ux_workers_render,
        },
    };
}

static void ux_workers_free(void *state)
{
    struct ux_workers *ux = state;
    ui_panel_free(ux->panel);
    ui_histo_free(&ux->histo);
    free(ux);
}

void ux_workers_show(struct coord star)
{
    struct ux_workers *ux = ux_state(ux_view_workers);

    ux->star = star;
    if (coord_is_nil(ux->star)) { ux_hide(ux_view_workers); return; }

    ux_workers_update(ux);
    ux_show(ux_view_workers);
    proxy_steps(cmd_steps_workers);
}

static void ux_workers_hide(void *)
{
    proxy_steps(cmd_steps_nil);
}

static void ux_workers_update(void *state)
{
    struct ux_workers *ux = state;
    ui_histo_update_legend(&ux->histo);

    world_ts t = 0;
    struct workers workers = {0};

    while (proxy_steps_next_workers(&t, &workers)) {
        if (ux->last_t == t) continue;
        ux->last_t = t;

        ui_histo_advance(&ux->histo, t);

        ui_histo_push(&ux->histo, ux_workers_queue, workers.queue);
        ui_histo_push(&ux->histo, ux_workers_work,
                workers.count - (workers.idle + workers.fail + workers.clean));
        ui_histo_push(&ux->histo, ux_workers_clean, workers.clean);
        ui_histo_push(&ux->histo, ux_workers_fail, workers.fail);
        ui_histo_push(&ux->histo, ux_workers_idle, workers.idle);
    }
}

static void ux_workers_event(void *state)
{
    struct ux_workers *ux = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_eq(ux->star, ev->star))
            ux_workers_show(ev->star);

    ui_histo_event(&ux->histo);
}

static void ux_workers_render(void *state, struct ui_layout *layout)
{
    struct ux_workers *ux = state;
    ui_histo_render(&ux->histo, layout);
}
