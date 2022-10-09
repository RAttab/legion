/* ui_status.c
   Rémi Attab (remi.attab@gmail.com), 20 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "utils/str.h"


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


struct ui_status *ui_status_new(void)
{
    struct dim dim = { .w = render.rect.w, .h = ui_st.font.base->glyph_h + 8 };
    struct pos pos = { .x = 0, .y = render.rect.h - dim.h };

    struct ui_status *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_status) {
        .panel = ui_panel_menu(pos, dim),
        .status = ui_label_new(ui_str_v(ui_status_cap)),
    };

    return ui;
}

void ui_status_free(struct ui_status *ui) {
    ui_panel_free(ui->panel);
    ui_label_free(&ui->status);
    free(ui);
}

int16_t ui_status_height(void)
{
    return render.ui.status->panel->w.dim.h;
}

void ui_status_set(
        struct ui_status *ui,
        enum status_type type,
        const char *msg,
        size_t len)
{
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


bool ui_status_event(struct ui_status *ui, SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(ui->panel, ev))) return ret != ui_skip;
    return ui_panel_event_consume(ui->panel, ev);
}

void ui_status_render(struct ui_status *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    time_sys delta = ts_now() - ui->ts;
    if (delta > ui_status_duration + ui_status_fade) ui->ts = 0;
    if (!ui->ts) return;

    if (delta > ui_status_duration) {
        delta -= ui_status_duration;
        ui->status.s.fg.a = 0xFF - ((0xFF * delta) / ui_status_fade);
    }
    else ui->status.s.fg.a = 0xFF;

    ui_label_render(&ui->status, &layout, renderer);
}
