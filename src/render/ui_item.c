/* ui_item.c
   Rémi Attab (remi.attab@gmail.com), 23 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// ???
// -----------------------------------------------------------------------------

static struct font *ui_item_font(void) { return font_mono6; }

#include "render/ui_progable.c"


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

struct ui_brain
{

};


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

struct ui_db
{

};


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

struct ui_worker
{

};


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

union ui_item_state
{
    struct progable progable;
    struct brain brain;
    struct db db;
    struct worker worker;

    // Some item have flexible sizes so we need to ensure enough room for all of
    // em.
    uint8_t _cap[item_state_len_max];
};

struct ui_item
{
    id_t id;
    struct coord star;
    union ui_item_state state;

    struct ui_panel panel;
    struct ui_progable progable;
    struct ui_brain brain;
    struct ui_db db;
    struct ui_worker worker;
};

struct ui_item *ui_item_new(void)
{
    size_t width = 200;
    struct pos pos = make_pos(
            core.rect.w - width - ui_star_width(core.ui.star),
            ui_topbar_height(core.ui.topbar));
    struct dim dim = make_dim(width, core.rect.h - pos.y);

    struct ui_item *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_item) {
        .panel = ui_panel_title(pos, dim, ui_str_v(id_str_len + 8)),
    };

    ui_progable_init(&ui->progable);

    ui->panel.state = ui_panel_hidden;
    return ui;
}

void ui_item_free(struct ui_item *ui)
{
    ui_panel_free(&ui->panel);
    ui_progable_free(&ui->progable);
    free(ui);
}

static void ui_item_update(struct ui_item *ui)
{
    if (!ui->id || coord_is_nil(ui->star)) return;

    struct chunk *chunk = sector_chunk(core.state.sector, ui->star);
    assert(chunk);

    bool ok = chunk_copy(chunk, ui->id, &ui->state, sizeof(ui->state));
    assert(ok);

    {
        char str[id_str_len];
        id_str(ui->id, sizeof(str), str);
        ui_str_setf(&ui->panel.title.str, "item - %s", str);
    }

    switch(id_item(ui->id))
    {
    case ITEM_WORKER: { assert(false); }

    case ITEM_PRINTER:
    case ITEM_MINER:
    case ITEM_DEPLOYER:
        return ui_progable_update(&ui->progable, &ui->state.progable);

    case ITEM_BRAIN_S:
    case ITEM_BRAIN_M:
    case ITEM_BRAIN_L: { assert(false); }

    case ITEM_DB_S:
    case ITEM_DB_M:
    case ITEM_DB_L: { assert(false); }

    default: { assert(false && "unsuported type in ui update"); }
    }
}

static bool ui_item_event_user(struct ui_item *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_ITEM_SELECT: {
        ui->id = (uintptr_t) ev->user.data1;
        ui->star = id_to_coord((uintptr_t) ev->user.data2);
        ui_item_update(ui);
        ui->panel.state = ui_panel_visible;
        core_push_event(EV_FOCUS, (uintptr_t) &ui->panel, 0);
        return true;
    }

    case EV_ITEM_CLEAR: {
        ui->id = 0;
        ui->panel.state = ui_panel_hidden;
        core_push_event(EV_FOCUS, 0, 0);
        return true;
    }

    case EV_STATE_UPDATE: {
        if (ui->panel.state == ui_panel_hidden) return false;
        ui_item_update(ui);
        return false;
    }

    default: { return false; }
    }
}

bool ui_item_event(struct ui_item *ui, SDL_Event *ev)
{
    if (ev->type == core.event && ui_item_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret == ui_consume;

    switch(id_item(ui->id))
    {
    case ITEM_WORKER: { assert(false); }

    case ITEM_PRINTER:
    case ITEM_MINER:
    case ITEM_DEPLOYER:
        return ui_progable_event(&ui->progable, &ui->state.progable, ev);

    case ITEM_BRAIN_S:
    case ITEM_BRAIN_M:
    case ITEM_BRAIN_L: { assert(false); }

    case ITEM_DB_S:
    case ITEM_DB_M:
    case ITEM_DB_L: { assert(false); }

    default: { assert(false && "unsuported type in ui event"); }
    }

    return false;
}

void ui_item_render(struct ui_item *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    switch(id_item(ui->id))
    {
    case ITEM_WORKER: { assert(false); }

    case ITEM_PRINTER:
    case ITEM_MINER:
    case ITEM_DEPLOYER:
        return ui_progable_render(&ui->progable, &ui->state.progable, &layout, renderer);

    case ITEM_BRAIN_S:
    case ITEM_BRAIN_M:
    case ITEM_BRAIN_L: { assert(false); }

    case ITEM_DB_S:
    case ITEM_DB_M:
    case ITEM_DB_L: { assert(false); }

    default: { assert(false && "unsuported type in ui render"); }
    }
}
