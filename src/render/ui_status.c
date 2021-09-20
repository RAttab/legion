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
static const ts_t ui_status_duration = 5 * ts_sec;

struct ui_status
{
    ts_t ts;
    struct ui_panel panel;
    struct ui_label status;
};


struct ui_status *ui_status_new(void)
{
    struct font *font = font_mono6;

    struct dim dim = { .w = core.rect.w, .h = font->glyph_h + 8 };
    struct pos pos = { .x = 0, .y = core.rect.h - dim.h };

    struct ui_status *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_status) {
        .panel = ui_panel_menu(pos, dim),
        .status = ui_label_new(font, ui_str_v(ui_status_cap)),
    };

    return ui;
}

void ui_status_free(struct ui_status *ui) {
    ui_panel_free(&ui->panel);
    ui_label_free(&ui->status);
    free(ui);
}

int16_t ui_status_height(void)
{
    return core.ui.status->panel.w.dim.h;
}

void ui_status_set(
        struct ui_status *ui,
        enum status type,
        const char *msg,
        size_t len)
{
    ui->ts = ts_now();

    switch (type)
    {
    case st_info: { ui->status.fg = rgba_white(); break; }
    case st_warn: { ui->status.fg = rgba_yellow(); break; }
    case st_error: { ui->status.fg = rgba_red(); break; }
    default: { assert(false); }
    }

    ui_str_setv(&ui->status.str, msg, len);
}


bool ui_status_event(struct ui_status *ui, SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret == ui_consume;
    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_status_render(struct ui_status *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);

    ts_t delta = ts_now() - ui->ts;
    if (delta > ui_status_duration) ui->ts = 0;
    if (!ui->ts) return;

    ui_label_render(&ui->status, &layout, renderer);
}
