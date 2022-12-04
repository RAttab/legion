/* ui_worker.c
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

enum
{
    ui_worker_queue = 0,
    ui_worker_work,
    ui_worker_clean,
    ui_worker_fail,
    ui_worker_idle,
};

struct ui_worker
{
    struct coord star;
    world_ts last_t;

    struct ui_panel *panel;
    struct ui_histo histo;
};


struct ui_worker *ui_worker_new(void)
{
    size_t width = 80 * ui_st.font.dim.w;
    struct pos pos = make_pos(
            render.rect.w - width - ui_star_width(render.ui.star),
            ui_topbar_height());
    struct dim dim = make_dim(width, render.rect.h - pos.y - ui_status_height());

    struct ui_histo_series series[] = {
        [ui_worker_queue] = { 0, "queue", ui_st.rgba.worker.queue, true },
        [ui_worker_work] =  { 1, "work", ui_st.rgba.worker.work, true },
        [ui_worker_clean] = { 1, "clean", ui_st.rgba.worker.clean, true },
        [ui_worker_fail] =  { 1, "fail", ui_st.rgba.worker.fail, true },
        [ui_worker_idle] =  { 1, "idle", ui_st.rgba.worker.idle, true },
    };

    struct ui_worker *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_worker) {
        .star = coord_nil(),
        .panel = ui_panel_title(pos, dim, ui_str_c("worker")),
        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    ui_panel_hide(ui->panel);
    return ui;
}

void ui_worker_free(struct ui_worker *ui)
{
    ui_panel_free(ui->panel);
    ui_histo_free(&ui->histo);
    free(ui);
}

static void ui_worker_clear(struct ui_worker *ui)
{
    ui->star = coord_nil();
    ui_histo_clear(&ui->histo);
    ui_panel_hide(ui->panel);
}

void ui_worker_update_state(struct ui_worker *ui)
{
    if (!ui_panel_is_visible(ui->panel)) return;

    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    if (!chunk) return;

    world_ts t = proxy_time(render.proxy);
    if (ui->last_t == t) return;
    ui->last_t = t;

    struct workers workers = chunk_workers(chunk);
    ui_histo_advance(&ui->histo, t);

    ui_histo_push(&ui->histo, ui_worker_queue, workers.queue);
    ui_histo_push(&ui->histo, ui_worker_work,
            workers.count - (workers.idle + workers.fail + workers.clean));
    ui_histo_push(&ui->histo, ui_worker_clean, workers.clean);
    ui_histo_push(&ui->histo, ui_worker_fail, workers.fail);
    ui_histo_push(&ui->histo, ui_worker_idle, workers.idle);
}

static void ui_worker_update(struct ui_worker *ui)
{
    ui_histo_update_legend(&ui->histo);
}

static bool ui_worker_event_user(struct ui_worker *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(ui->panel)) return false;
        ui_worker_update(ui);
        return false;
    }

    case EV_WORKER_TOGGLE: {
        if (ui_panel_is_visible(ui->panel)) {
            ui_worker_clear(ui);
            return false;
        }

        ui->star = coord_from_u64((uintptr_t) ev->user.data1);
        assert(!coord_is_nil(ui->star));
        ui_worker_update(ui);
        ui_panel_show(ui->panel);
        return false;
    }

    case EV_STATE_LOAD:
    case EV_MAN_GOTO:
    case EV_MAN_TOGGLE:
    case EV_PILLS_TOGGLE:
    case EV_ENERGY_TOGGLE:
    case EV_STAR_CLEAR:
    case EV_ITEM_SELECT: { ui_worker_clear(ui); return false; }

    default: { return false; }
    }

}

bool ui_worker_event(struct ui_worker *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_worker_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(ui->panel, ev))) {
        if (ret == ui_consume && !ui_panel_is_visible(ui->panel))
            ui_worker_clear(ui);
        return ret != ui_skip;
    }

    if ((ret = ui_histo_event(&ui->histo, ev))) return true;

    return ui_panel_event_consume(ui->panel, ev);
}

void ui_worker_render(struct ui_worker *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_histo_render(&ui->histo, &layout, renderer);
}
