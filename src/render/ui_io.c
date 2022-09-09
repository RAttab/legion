/* ui_io.c
   Rémi Attab (remi.attab@gmail.com), 25 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "game/active.h"
#include "items/types.h"
#include "vm/atoms.h"
#include "render/ui.h"
#include "ui/ui.h"

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
    enum io id;
    bool active;

    struct ui_label name;
    size_t args;
    struct ui_io_arg arg[4];
    struct ui_button exec;
};

static struct ui_io_cmd ui_io_cmd(
        struct font *font, enum io id, size_t args)
{
    struct ui_io_cmd cmd = {
        .id = id,
        .active = false,
        .name = ui_label_new(font, ui_str_v(symbol_cap)),
        .exec = ui_button_new(font, ui_str_c("exec >>")),
        .args = args,
    };

    struct symbol str = {0};
    bool ok = atoms_str(proxy_atoms(render.proxy), id, &str);
    assert(ok);

    ui_str_set_symbol(&cmd.name.str, &str);
    cmd.name.w.dim.w = ui_layout_inf;
    cmd.name.bg = rgba_gray_a(0x44, 0x44);

    return cmd;
}

static struct ui_io_cmd ui_io_cmd0(struct font *font, enum io id)
{
    return ui_io_cmd(font, id, 0);
}

static struct ui_io_cmd ui_io_cmd1(
        struct font *font, enum io id, const char *arg)
{
    struct ui_io_cmd cmd = ui_io_cmd(font, id, 1);
    cmd.arg[0] = ui_io_arg(font, arg);
    return cmd;
}

static struct ui_io_cmd ui_io_cmd2(
        struct font *font, enum io id, const char *arg0, const char *arg1)
{
    struct ui_io_cmd cmd = ui_io_cmd(font, id, 2);
    cmd.arg[0] = ui_io_arg(font, arg0);
    cmd.arg[1] = ui_io_arg(font, arg1);
    return cmd;
}

static struct ui_io_cmd ui_io_cmd3(
        struct font *font, enum io id,
        const char *arg0, const char *arg1, const char *arg2)
{
    struct ui_io_cmd cmd = ui_io_cmd(font, id, 3);
    cmd.arg[0] = ui_io_arg(font, arg0);
    cmd.arg[1] = ui_io_arg(font, arg1);
    cmd.arg[2] = ui_io_arg(font, arg2);
    return cmd;
}

static struct ui_io_cmd ui_io_cmd4(
        struct font *font, enum io id,
        const char *arg0, const char *arg1, const char *arg2, const char *arg3)
{
    struct ui_io_cmd cmd = ui_io_cmd(font, id, 4);
    cmd.arg[0] = ui_io_arg(font, arg0);
    cmd.arg[1] = ui_io_arg(font, arg1);
    cmd.arg[2] = ui_io_arg(font, arg2);
    cmd.arg[3] = ui_io_arg(font, arg3);
    return cmd;
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static struct font *ui_io_font(void) { return font_mono6; }

enum
{
    ui_io_reset = 0,
    ui_io_item,
    ui_io_tape,
    ui_io_mod,

    ui_io_name,
    ui_io_send,
    ui_io_dbg_attach,
    ui_io_dbg_detach,
    ui_io_dbg_break,
    ui_io_dbg_step,

    ui_io_set,
    ui_io_cas,
    ui_io_probe,
    ui_io_scan,
    ui_io_launch,
    ui_io_target,
    ui_io_value,

    ui_io_max,
};

struct ui_io
{
    im_id id;
    struct coord star;

    size_t list_len;
    const vm_word *list;

    bool loading;

    struct ui_panel panel;
    struct ui_label target, target_val;

    struct ui_io_cmd io[ui_io_max];
};


struct ui_io *ui_io_new(void)
{
    struct font *font = ui_io_font();

    size_t width = 38 * font->glyph_w;
    struct pos pos = make_pos(
            render.rect.w - width - ui_item_width(render.ui.item) - ui_star_width(render.ui.star),
            ui_topbar_height());
    struct dim dim = make_dim(width, render.rect.h - pos.y - ui_status_height());

    struct ui_io *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_io) {
        .panel = ui_panel_title(pos, dim, ui_str_c("io")),
        .target = ui_label_new(font, ui_str_c("target: ")),
        .target_val = ui_label_new(font, ui_str_v(im_id_str_len)),
        .io = {
            [ui_io_reset] = ui_io_cmd0(font, IO_RESET),
            [ui_io_item] = ui_io_cmd2(font, IO_ITEM, "item:  ", "loops: "),
            [ui_io_tape] = ui_io_cmd2(font, IO_TAPE, "id:    ", "loops: "),
            [ui_io_mod] = ui_io_cmd1(font, IO_MOD,   "id:    "),

            [ui_io_name] = ui_io_cmd1(font, IO_NAME, "name:  "),
            [ui_io_send] = ui_io_cmd4(font, IO_SEND,
                    "len:   ", "[0]:   ", "[1]:   ", "[2]:   "),
            [ui_io_dbg_attach] = ui_io_cmd0(font, IO_DBG_ATTACH),
            [ui_io_dbg_detach] = ui_io_cmd0(font, IO_DBG_DETACH),
            [ui_io_dbg_break] = ui_io_cmd1(font, IO_DBG_BREAK, "ip:    "),
            [ui_io_dbg_step] = ui_io_cmd0(font, IO_DBG_STEP),

            [ui_io_set] = ui_io_cmd2(font, IO_SET,   "index: ", "value: "),
            [ui_io_cas] = ui_io_cmd3(font, IO_CAS,   "index: ", "test:  ", "value: "),
            [ui_io_probe] = ui_io_cmd2(font, IO_PROBE, "item:  ", "coord: "),
            [ui_io_scan] = ui_io_cmd1(font, IO_SCAN, "coord: "),
            [ui_io_launch] = ui_io_cmd1(font, IO_LAUNCH, "dest:  "),
            [ui_io_target] = ui_io_cmd1(font, IO_TARGET, "dest:  "),
            [ui_io_value] = ui_io_cmd0(font, IO_VALUE),
        },
    };

    ui_panel_hide(&ui->panel);
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

    const struct im_config *config = im_config_assert(im_id_item(ui->id));
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
        ui_panel_hide(&ui->panel);
        ui->id = 0;
        ui->star = coord_nil();
        ui->list_len = 0;
        ui->list = NULL;
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        if (coord_is_nil(ui->star)) return false;
        ui->loading = !proxy_chunk(render.proxy, ui->star);
        return false;
    }

    case EV_IO_TOGGLE: {
        if (ui_panel_is_visible(&ui->panel))
            ui_panel_hide(&ui->panel);
        else ui_panel_show(&ui->panel);
        return false;
    }

    case EV_ITEM_SELECT: {
        ui->id = (uintptr_t) ev->user.data1;
        ui->star = coord_from_u64((uintptr_t) ev->user.data2);
        ui_io_update(ui);
        return false;
    }

    case EV_STAR_SELECT: {
        struct coord new = coord_from_u64((uintptr_t) ev->user.data1);
        if (!coord_eq(ui->star, new)) {
            ui_panel_hide(&ui->panel);
            ui->id = 0;
        }
        return false;
    }

    case EV_MAN_GOTO:
    case EV_MAN_TOGGLE:
    case EV_STAR_CLEAR:
    case EV_ITEM_CLEAR: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    default: { return false; }
    }
}

static void ui_io_exec(struct ui_io *ui, struct ui_io_cmd *cmd)
{
    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    assert(chunk);

    size_t len = cmd->args;
    vm_word args[cmd->args];
    memset(args, 0, sizeof(args));

    switch (cmd->id)
    {
    case IO_SEND: {
        vm_word val = 0;
        if (!ui_input_eval(&cmd->arg[0].val, &val)) return;
        len = legion_min(val, im_packet_max);
        len = legion_max(val, 0);

        for (size_t i = 0; i < len; ++i) {
            if (!ui_input_eval(&cmd->arg[i+1].val, &val)) return;
            args[i] = val;
        }
        break;
    }

    default: {
        vm_word val = 0;
        for (size_t i = 0; i < cmd->args; ++i) {
            if (!ui_input_eval(&cmd->arg[i].val, &val)) return;
            args[i] = val;
        }
        break;
    }
    }

    proxy_io(render.proxy, cmd->id, ui->id, args, len);
}

bool ui_io_event(struct ui_io *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_io_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret != ui_skip;

    if (ui->loading) return ui_panel_event_consume(&ui->panel, ev);

    for (size_t i = 0; i < ui_io_max; ++i) {
        struct ui_io_cmd *cmd = &ui->io[i];
        if (!cmd->active) continue;

        for (size_t j = 0; j < cmd->args; ++j) {
            if (!(ret = ui_input_event(&cmd->arg[j].val, ev))) continue;
            if (ret != ui_action) return true;

            if (j + 1 == cmd->args) ui_io_exec(ui, cmd);
            else  {
                struct ui_input *next = &cmd->arg[j+1].val;
                render_push_event(EV_FOCUS_INPUT, (uintptr_t) next, 0);
            }
            return true;
        }

        if ((ret = ui_button_event(&cmd->exec, ev))) {
            ui_io_exec(ui, cmd);
            return true;
        }
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_io_render(struct ui_io *ui, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;
    if (ui->loading) return;

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
