/* ui_status.c
   Rémi Attab (remi.attab@gmail.com), 20 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "utils/str.h"

static void ui_status_free(void *);
static void ui_status_render(void *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------


static const size_t ui_status_cap = ui_str_cap;
static const time_sys ui_status_duration = 5 * ts_sec;
static const time_sys ui_status_fade = 100 * ts_msec;

struct ui_status
{
    time_sys ts;
    struct ui_panel *panel;
    struct ui_label status;
};


void ui_status_alloc(struct ui_view_state *state)
{
    struct ui_status *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_status) {
        .panel = ui_panel_menu(make_dim(
                        ui_layout_inf,
                        ui_st.font.base->glyph_h + 8)),
        .status = ui_label_new(ui_str_v(ui_status_cap)),
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_status,
        .panel = ui->panel,
        .fn = {
            .free = ui_status_free,
            .render = ui_status_render,
        },
    };
}

static void ui_status_free(void *state) {
    struct ui_status *ui = state;
    ui_panel_free(ui->panel);
    ui_label_free(&ui->status);
    free(ui);
}

void ui_status_set(enum status_type type, const char *msg, size_t len)
{
    struct ui_status *ui = ui_state(ui_view_status);

    ui->ts = ts_now();

    switch (type)
    {
    case st_info: { ui->status.s.fg = ui_st.rgba.info; break; }
    case st_warn: { ui->status.s.fg = ui_st.rgba.warn; break; }
    case st_error: { ui->status.s.fg = ui_st.rgba.error; break; }
    default: { assert(false); }
    }

    ui_str_setv(&ui->status.str, msg, len);
}

static void ui_status_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_status *ui = state;

    time_sys delta = ts_now() - ui->ts;
    if (delta > ui_status_duration + ui_status_fade) ui->ts = 0;
    if (!ui->ts) return;

    if (delta > ui_status_duration) {
        delta -= ui_status_duration;
        ui->status.s.fg.a = 0xFF - ((0xFF * delta) / ui_status_fade);
    }
    else ui->status.s.fg.a = 0xFF;

    ui_label_render(&ui->status, layout, renderer);
}
