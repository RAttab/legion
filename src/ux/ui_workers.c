/* ui_workers.c
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ux/ui.h"
#include "game/chunk.h"

static void ui_workers_free(void *);
static void ui_workers_update_state(void *);
static void ui_workers_update_frame(void *);
static void ui_workers_event(void *);
static void ui_workers_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

enum
{
    ui_workers_queue = 0,
    ui_workers_work,
    ui_workers_clean,
    ui_workers_fail,
    ui_workers_idle,
};

struct ui_workers
{
    struct coord star;
    world_ts last_t;

    struct ui_panel *panel;
    struct ui_histo histo;
};


void ui_workers_alloc(struct ui_view_state *state)
{
    struct ui_histo_series series[] = {
        [ui_workers_queue] = { 0, "queue", ui_st.rgba.worker.queue, true },
        [ui_workers_work] =  { 1, "work", ui_st.rgba.worker.work, true },
        [ui_workers_clean] = { 1, "clean", ui_st.rgba.worker.clean, true },
        [ui_workers_fail] =  { 1, "fail", ui_st.rgba.worker.fail, true },
        [ui_workers_idle] =  { 1, "idle", ui_st.rgba.worker.idle, true },
    };

    struct ui_workers *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_workers) {
        .star = coord_nil(),

        .panel = ui_panel_title(
                make_dim(77 * engine_cell().w, ui_layout_inf),
                ui_str_c("workers")),

        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_workers,
        .parent = ui_view_star,
        .slots = ui_slot_right_sub,
        .panel = ui->panel,
        .fn = {
            .free = ui_workers_free,
            .update_state = ui_workers_update_state,
            .update_frame = ui_workers_update_frame,
            .event = ui_workers_event,
            .render = ui_workers_render,
        },
    };
}

static void ui_workers_free(void *state)
{
    struct ui_workers *ui = state;
    ui_panel_free(ui->panel);
    ui_histo_free(&ui->histo);
    free(ui);
}

void ui_workers_show(struct coord star)
{
    struct ui_workers *ui = ui_state(ui_view_workers);

    ui->star = star;
    if (coord_is_nil(ui->star)) { ui_hide(ui_view_workers); return; }

    ui_workers_update_frame(ui);
    ui_show(ui_view_workers);
}

static void ui_workers_update_state(void *state)
{
    struct ui_workers *ui = state;

    struct chunk *chunk = proxy_chunk(ui->star);
    if (!chunk) return;

    world_ts t = proxy_time();
    if (ui->last_t == t) return;
    ui->last_t = t;

    struct workers workers = chunk_workers(chunk);
    ui_histo_advance(&ui->histo, t);

    ui_histo_push(&ui->histo, ui_workers_queue, workers.queue);
    ui_histo_push(&ui->histo, ui_workers_work,
            workers.count - (workers.idle + workers.fail + workers.clean));
    ui_histo_push(&ui->histo, ui_workers_clean, workers.clean);
    ui_histo_push(&ui->histo, ui_workers_fail, workers.fail);
    ui_histo_push(&ui->histo, ui_workers_idle, workers.idle);
}

static void ui_workers_update_frame(void *state)
{
    struct ui_workers *ui = state;

    ui_histo_update_legend(&ui->histo);
}

static void ui_workers_event(void *state)
{
    struct ui_workers *ui = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_eq(ui->star, ev->star))
            ui_workers_show(ev->star);
    
    ui_histo_event(&ui->histo);
}

static void ui_workers_render(void *state, struct ui_layout *layout)
{
    struct ui_workers *ui = state;
    ui_histo_render(&ui->histo, layout);
}
