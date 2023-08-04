/* ui_workers.c
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"

static void ui_workers_free(void *);
static void ui_workers_update_state(void *);
static void ui_workers_update_frame(void *);
static bool ui_workers_event(void *, SDL_Event *);
static void ui_workers_render(void *, struct ui_layout *, SDL_Renderer *);


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
                make_dim(77 * ui_st.font.dim.w, ui_layout_inf),
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

static void ui_workers_event_user(struct ui_workers *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case ev_star_select: {
        struct coord new = coord_from_u64((uintptr_t) ev->user.data1);
        if (!coord_eq(ui->star, new)) ui_workers_show(new);
        return;
    }

    default: { return; }
    }
}

static bool ui_workers_event(void *state, SDL_Event *ev)
{
    struct ui_workers *ui = state;

    if (ev->type == render.event)
        ui_workers_event_user(ui, ev);
    
    enum ui_ret ret = ui_nil;
    if ((ret = ui_histo_event(&ui->histo, ev))) return true;

    return false;
}

static void ui_workers_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_workers *ui = state;

    ui_histo_render(&ui->histo, layout, renderer);
}
