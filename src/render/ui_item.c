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
// global
// -----------------------------------------------------------------------------

static struct font *ui_item_font(void) { return font_mono6; }

#include "render/items/ui_prog.c"
#include "render/items/ui_deploy.c"
#include "render/items/ui_extract.c"
#include "render/items/ui_printer.c"
#include "render/items/ui_storage.c"
#include "render/items/ui_brain.c"
#include "render/items/ui_db.c"
#include "render/items/ui_legion.c"


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

union ui_item_state
{
    struct deploy deploy;
    struct extract extract;
    struct printer printer;
    struct storage storage;
    struct brain brain;
    struct db db;
    struct legion legion;

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
    struct ui_button io;
    struct ui_label id_lbl;
    struct ui_link id_val;

    struct ui_deploy deploy;
    struct ui_extract extract;
    struct ui_printer printer;
    struct ui_storage storage;
    struct ui_brain brain;
    struct ui_db db;
    struct ui_legion legion;
};

struct ui_item *ui_item_new(void)
{
    struct font *font = ui_item_font();

    size_t width = 34 * ui_item_font()->glyph_w;
    struct pos pos = make_pos(
            core.rect.w - width - ui_star_width(core.ui.star),
            ui_topbar_height(core.ui.topbar));
    struct dim dim = make_dim(width, core.rect.h - pos.y);

    struct ui_item *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_item) {
        .panel = ui_panel_title(pos, dim, ui_str_v(id_str_len + 8)),
        .io = ui_button_new(font, ui_str_c("<< io")),
        .id_lbl = ui_label_new(font, ui_str_c("id: ")),
        .id_val = ui_link_new(font, ui_str_v(id_str_len)),
    };

    ui->io.w.dim.w = ui_layout_inf;

    ui_deploy_init(&ui->deploy);
    ui_extract_init(&ui->extract);
    ui_printer_init(&ui->printer);
    ui_storage_init(&ui->storage);
    ui_brain_init(&ui->brain);
    ui_db_init(&ui->db);
    ui_legion_init(&ui->legion);

    ui->panel.state = ui_panel_hidden;
    return ui;
}

void ui_item_free(struct ui_item *ui)
{
    ui_panel_free(&ui->panel);
    ui_button_free(&ui->io);
    ui_label_free(&ui->id_lbl);
    ui_link_free(&ui->id_val);
    ui_deploy_free(&ui->deploy);
    ui_extract_free(&ui->extract);
    ui_printer_free(&ui->printer);
    ui_storage_free(&ui->storage);
    ui_brain_free(&ui->brain);
    ui_db_free(&ui->db);
    ui_legion_free(&ui->legion);
    free(ui);
}

int16_t ui_item_width(struct ui_item *ui)
{
    return ui->panel.w.dim.w;
}

static void ui_item_update(struct ui_item *ui)
{
    if (!ui->id || coord_is_nil(ui->star)) return;

    ui_str_set_id(&ui->id_val.str, ui->id);

    struct chunk *chunk = world_chunk(core.state.world, ui->star);
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
    case ITEM_DEPLOY:
        return ui_deploy_update(&ui->deploy, &ui->state.deploy);
    case ITEM_EXTRACT_1...ITEM_EXTRACT_3:
        return ui_extract_update(&ui->extract, &ui->state.extract);
    case ITEM_PRINTER_1...ITEM_ASSEMBLY_3:
        return ui_printer_update(&ui->printer, &ui->state.printer);
    case ITEM_STORAGE:
        return ui_storage_update(&ui->storage, &ui->state.storage);
    case ITEM_BRAIN_1...ITEM_BRAIN_3:
        return ui_brain_update(&ui->brain, &ui->state.brain);
    case ITEM_DB_1...ITEM_DB_3:
        return ui_db_update(&ui->db, &ui->state.db);
    case ITEM_LEGION_1...ITEM_LEGION_3:
        return ui_legion_update(&ui->legion, &ui->state.legion);
    default: { assert(false && "unsuported type in ui update"); }
    }
}

static bool ui_item_event_user(struct ui_item *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui->panel.state = ui_panel_hidden;
        ui->id = 0;
        ui->star = coord_nil();
        return false;
    }

    case EV_ITEM_SELECT: {
        ui->id = (uintptr_t) ev->user.data1;
        ui->star = id_to_coord((uintptr_t) ev->user.data2);
        ui_item_update(ui);
        ui->panel.state = ui_panel_visible;
        core_push_event(EV_FOCUS_PANEL, (uintptr_t) &ui->panel, 0);
        return false;
    }

    case EV_ITEM_CLEAR: {
        ui->id = 0;
        ui->panel.state = ui_panel_hidden;
        core_push_event(EV_FOCUS_PANEL, 0, 0);
        return false;
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
    if ((ret = ui_panel_event(&ui->panel, ev))) {
        if (ret == ui_consume && ui->panel.state == ui_panel_hidden)
            core_push_event(EV_ITEM_CLEAR, 0, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->io, ev))) {
        core_push_event(EV_IO_TOGGLE, ui->id, coord_to_id(ui->star));
        return true;
    }

    if ((ret = ui_link_event(&ui->id_val, ev))) {
        ui_clipboard_copy_hex(&core.ui.board, ui->id);
        return true;
    }

    switch(id_item(ui->id))
    {
    case ITEM_DEPLOY:
        return ui_deploy_event(&ui->deploy, &ui->state.deploy, ev);
    case ITEM_EXTRACT_1...ITEM_EXTRACT_3:
        return ui_extract_event(&ui->extract, &ui->state.extract, ev);
    case ITEM_PRINTER_1...ITEM_ASSEMBLY_3:
        return ui_printer_event(&ui->printer, &ui->state.printer, ev);
    case ITEM_STORAGE:
        return ui_storage_event(&ui->storage, &ui->state.storage, ev);
    case ITEM_BRAIN_1...ITEM_BRAIN_3:
        return ui_brain_event(&ui->brain, &ui->state.brain, ev);
    case ITEM_DB_1...ITEM_DB_3:
        return ui_db_event(&ui->db, &ui->state.db, ev);
    case ITEM_LEGION_1...ITEM_LEGION_3:
        return ui_legion_event(&ui->legion, &ui->state.legion, ev);
    default: { assert(false && "unsuported type in ui update"); }
    }

    return false;
}

void ui_item_render(struct ui_item *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_button_render(&ui->io, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_label_render(&ui->id_lbl, &layout, renderer);
    ui_link_render(&ui->id_val, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_y(&layout, ui_item_font()->glyph_h);

    switch(id_item(ui->id))
    {
    case ITEM_DEPLOY:
        return ui_deploy_render(&ui->deploy, &ui->state.deploy, &layout, renderer);
    case ITEM_EXTRACT_1...ITEM_EXTRACT_3:
        return ui_extract_render(&ui->extract, &ui->state.extract, &layout, renderer);
    case ITEM_PRINTER_1...ITEM_ASSEMBLY_3:
        return ui_printer_render(&ui->printer, &ui->state.printer, &layout, renderer);
    case ITEM_STORAGE:
        return ui_storage_render(&ui->storage, &ui->state.storage, &layout, renderer);
    case ITEM_BRAIN_1...ITEM_BRAIN_3:
        return ui_brain_render(&ui->brain, &ui->state.brain, &layout, renderer);
    case ITEM_DB_1...ITEM_DB_3:
        return ui_db_render(&ui->db, &ui->state.db, &layout, renderer);
    case ITEM_LEGION_1...ITEM_LEGION_3:
        return ui_legion_render(&ui->legion, &ui->state.legion, &layout, renderer);
    default: { assert(false && "unsuported type in ui update"); }
    }
}
