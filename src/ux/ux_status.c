/* ux_status.c
   Rémi Attab (remi.attab@gmail.com), 20 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

static void ux_status_free(void *);
static void ux_status_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------


static const size_t ux_status_cap = ui_str_cap;
static const sys_ts ux_status_duration = 5 * sys_sec;
static const sys_ts ux_status_fade = 100 * sys_msec;

struct ux_status
{
    sys_ts ts;
    struct ui_panel *panel;
    struct ui_label status;
};


void ux_status_alloc(struct ux_view_state *state)
{
    struct ux_status *ux = mem_alloc_t(ux);

    struct dim cell = engine_cell();
    *ux = (struct ux_status) {
        .panel = ui_panel_menu(make_dim(ui_layout_inf, cell.h)),
        .status = ui_label_new(ui_str_v(ux_status_cap)),
    };

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_status,
        .panel = ux->panel,
        .fn = {
            .free = ux_status_free,
            .render = ux_status_render,
        },
    };
}

static void ux_status_free(void *state) {
    struct ux_status *ux = state;
    ui_panel_free(ux->panel);
    ui_label_free(&ux->status);
    mem_free(ux);
}

void ux_status_set(enum status_type type, const char *msg, size_t len)
{
    struct ux_status *ux = ux_state(ux_view_status);

    ux->ts = sys_now();

    switch (type)
    {
    case st_info: { ux->status.s.fg = ui_st.rgba.info; break; }
    case st_warn: { ux->status.s.fg = ui_st.rgba.warn; break; }
    case st_error: { ux->status.s.fg = ui_st.rgba.error; break; }
    default: { assert(false); }
    }

    ui_str_setv(&ux->status.str, msg, len);
}

static void ux_status_render(void *state, struct ui_layout *layout)
{
    struct ux_status *ux = state;

    sys_ts delta = sys_now() - ux->ts;
    if (delta > ux_status_duration + ux_status_fade) ux->ts = 0;
    if (!ux->ts) return;

    if (delta > ux_status_duration) {
        delta -= ux_status_duration;
        ux->status.s.fg.a = 0xFF - ((0xFF * delta) / ux_status_fade);
    }
    else ux->status.s.fg.a = 0xFF;

    ui_label_render(&ux->status, layout);
}
