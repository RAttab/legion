/* ui_io.c
   Rémi Attab (remi.attab@gmail.com), 25 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "game/active.h"

// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

struct ui_io_arg
{
    struct ui_label name;
    struct ui_input val;
};

static struct ui_io_arg ui_io_arg(struct font *font, const char *arg)
{
    return (struct ui_io_arg) {
        .name = ui_label_new(font, ui_str_c(arg)),
        .val = ui_input_new(font, 20),
    };
}

struct ui_io_cmd
{
    enum atom_io id;
    bool active;

    struct ui_label name;
    size_t args;
    struct ui_io_arg arg[2];
    struct ui_button exec;
};

static struct ui_io_cmd ui_io_cmd(
        struct font *font, enum atom_io id, size_t args)
{
    struct ui_io_cmd cmd = {
        .id = id,
        .active = false,
        .name = ui_label_new(font, ui_str_v(vm_atom_cap)),
        .exec = ui_button_new(font, ui_str_c("exec >>")),
        .args = args,
    };

    atom_t str = {0};
    bool ok = vm_atoms_str(id, &str);
    assert(ok);

    ui_str_setc(&cmd.name.str, str);
    cmd.name.w.dim.w = ui_layout_inf;
    cmd.name.bg = rgba_gray_a(0x44, 0x44);

    return cmd;
}

static struct ui_io_cmd ui_io_cmd0(struct font *font, enum atom_io id)
{
    return ui_io_cmd(font, id, 0);
}

static struct ui_io_cmd ui_io_cmd1(
        struct font *font, enum atom_io id, const char *arg)
{
    struct ui_io_cmd cmd = ui_io_cmd(font, id, 1);
    cmd.arg[0] = ui_io_arg(font, arg);
    return cmd;
}

static struct ui_io_cmd ui_io_cmd2(
        struct font *font, enum atom_io id, const char *arg0, const char *arg1)
{
    struct ui_io_cmd cmd = ui_io_cmd(font, id, 2);
    cmd.arg[0] = ui_io_arg(font, arg0);
    cmd.arg[1] = ui_io_arg(font, arg1);
    return cmd;
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static struct font *ui_io_font(void) { return font_mono6; }

enum
{
    ui_io_reset = 0,
    ui_io_prog,
    ui_io_mod,
    ui_io_set,

    ui_io_max,
};

struct ui_io
{
    id_t id;
    struct coord star;

    size_t list_len;
    const word_t *list;

    struct ui_panel panel;
    struct ui_label target, target_val;

    struct ui_io_cmd io[ui_io_max];
};


struct ui_io *ui_io_new(void)
{
    struct font *font = ui_io_font();

    size_t width = 34 * font->glyph_w;
    struct pos pos = make_pos(
            core.rect.w - width - ui_item_width(core.ui.item) - ui_star_width(core.ui.star),
            ui_topbar_height(core.ui.topbar));
    struct dim dim = make_dim(width, core.rect.h - pos.y);

    struct ui_io *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_io) {
        .panel = ui_panel_title(pos, dim, ui_str_c("io")),
        .target = ui_label_new(font, ui_str_c("target: ")),
        .target_val = ui_label_new(font, ui_str_v(id_str_len)),
        .io = {
            [ui_io_reset] = ui_io_cmd0(font, IO_RESET),
            [ui_io_prog] = ui_io_cmd2(font, IO_PROG, "id:    ", "loops: "),
            [ui_io_mod] = ui_io_cmd1(font, IO_MOD, "id:    "),
            [ui_io_set] = ui_io_cmd2(font, IO_SET, "index: ", "value: "),
        },
    };

    for (size_t i = 0; i < ui_io_max; ++i) {
    }

    ui->panel.state = ui_panel_hidden;
    return ui;
}

void ui_io_free(struct ui_io *ui)
{
    ui_panel_free(&ui->panel);
    ui_label_free(&ui->target);
    ui_label_free(&ui->target_val);

    for (size_t i = 0; i < ui_io_max; ++i) {
        struct ui_io_cmd *cmd = &ui->io[i];
        ui_label_free(&cmd->name);
        ui_button_free(&cmd->exec);
        for (size_t j = 0; j < cmd->args; ++j) {
            ui_label_free(&cmd->arg[j].name);
            ui_input_free(&cmd->arg[j].val);
        }
    }

    free(ui);
}

static void ui_io_update(struct ui_io *ui)
{
    ui_str_set_id(&ui->target_val.str, ui->id);

    const struct item_config *config = item_config(id_item(ui->id));
    ui->list = config->io_list;
    ui->list_len = config->io_list_len;

    for (size_t i = 0; i < ui_io_max; ++i) {
        struct ui_io_cmd *cmd = &ui->io[i];

        for (size_t j = 0; j < cmd->args; ++j)
            ui_input_clear(&cmd->arg[j].val);

        cmd->active = false;
        for (size_t k = 0; k < ui->list_len; ++k)
            cmd->active = cmd->active || (cmd->id == ui->list[k]);
    }
}

static bool ui_io_event_user(struct ui_io *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui->panel.state = ui_panel_hidden;
        ui->id = 0;
        ui->star = coord_nil();
        ui->list_len = 0;
        ui->list = NULL;
        return false;
    }

    case EV_IO_TOGGLE: {
        if (ui->panel.state != ui_panel_hidden) {
            ui->panel.state = ui_panel_hidden;
            return false;
        }
        ui->panel.state = ui_panel_visible;
        core_push_event(EV_FOCUS_PANEL, (uintptr_t) &ui->panel, 0);
        return false;
    }

    case EV_ITEM_SELECT: {
        ui->id = (uintptr_t) ev->user.data1;
        ui->star = id_to_coord((uintptr_t) ev->user.data2);
        ui_io_update(ui);
        return false;
    }

    case EV_STAR_CLEAR: // fallthrough
    case EV_ITEM_CLEAR: {
        ui->panel.state = ui_panel_hidden;
        return false;
    }

    default: { return false; }
    }
}

static void ui_io_exec(struct ui_io *ui, struct ui_io_cmd *cmd)
{
    struct chunk *chunk = world_chunk(core.state.world, ui->star);
    assert(chunk);

    word_t args[cmd->args];
    for (size_t i = 0; i < cmd->args; ++i)
        args[i] = ui_input_get_u64(&cmd->arg[i].val);

    bool ok = chunk_io(chunk, cmd->id, 0, ui->id, cmd->args, args);
    assert(ok);
}

bool ui_io_event(struct ui_io *ui, SDL_Event *ev)
{
    if (ev->type == core.event && ui_io_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret == ui_consume;

    for (size_t i = 0; i < ui_io_max; ++i) {
        struct ui_io_cmd *cmd = &ui->io[i];
        if (!cmd->active) continue;

        for (size_t j = 0; j < cmd->args; ++j)
            if ((ret = ui_input_event(&cmd->arg[j].val, ev)))
                return ret == ui_consume;

        if ((ret = ui_button_event(&cmd->exec, ev))) {
            ui_io_exec(ui, cmd);
            return ret == ui_consume;
        }
    }

    return false;
}

void ui_io_render(struct ui_io *ui, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_label_render(&ui->target, &layout, renderer);
    ui_label_render(&ui->target_val, &layout, renderer);
    ui_layout_next_row(&layout);
    ui_layout_sep_y(&layout, font->glyph_h);

    for (size_t i = 0; i < ui_io_max; ++i) {
        struct ui_io_cmd *cmd = &ui->io[i];
        if (!cmd->active) continue;

        ui_label_render(&cmd->name, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, 2);

        for (size_t j = 0; j < cmd->args; ++j) {
            struct ui_io_arg *arg = &cmd->arg[j];
            ui_layout_sep_x(&layout, 2 * font->glyph_w);
            ui_label_render(&arg->name, &layout, renderer);
            ui_input_render(&arg->val, &layout, renderer);
            ui_layout_next_row(&layout);
        }

        ui_layout_sep_x(&layout, 2 * font->glyph_w);
        ui_button_render(&cmd->exec, &layout, renderer);
        ui_layout_next_row(&layout);

        ui_layout_sep_y(&layout, font->glyph_h);
    }
}
