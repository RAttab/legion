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
// io
// -----------------------------------------------------------------------------

enum
{
    ui_io_cmds_max = 32,
    ui_io_args_max = 5,
};

struct ui_io_arg
{
    bool required;
    struct ui_label name;
    struct ui_input val;
};

struct ui_io_cmd
{
    enum io io;

    struct ui_button name;
    struct ui_button help;

    size_t len;
    struct ui_io_arg args[ui_io_args_max];

    struct ui_button exec;
};

struct ui_io
{
    im_id id;
    struct coord star;

    bool loading;
    enum io open;

    struct ui_panel *panel;
    struct ui_label required;

    size_t len;
    struct ui_io_cmd cmds[ui_io_cmds_max];
};


struct ui_io *ui_io_new(void)
{
    size_t width = 38 * ui_st.font.dim.w;
    struct pos pos = make_pos(
            render.rect.w - width - ui_item_width(render.ui.item) - ui_star_width(render.ui.star),
            ui_topbar_height());
    struct dim dim = make_dim(width, render.rect.h - pos.y - ui_status_height());

    struct ui_io *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_io) {
        .open = io_nil,

        .panel = ui_panel_title(pos, dim, ui_str_c("io")),
        .required = ui_label_new_s(&ui_st.label.required, ui_str_c("*")),
    };

    for (size_t i = 0; i < ui_io_cmds_max; ++i) {
        struct ui_io_cmd *cmd = ui->cmds + i;
        *cmd = (struct ui_io_cmd) {
            .name = ui_button_new_s(&ui_st.button.list.close, ui_str_v(symbol_cap)),
            .help = ui_button_new(ui_str_c("?")),
            .exec = ui_button_new(ui_str_c("exec >>")),
        };

        cmd->name.w.dim.w = ui_layout_inf;

        for (size_t j = 0; j < ui_io_args_max; ++j) {
            cmd->args[j] = (struct ui_io_arg) {
                .name = ui_label_new(ui_str_v(10)),
                .val = ui_input_new(22),
            };
        }
    }

    ui_panel_hide(ui->panel);
    return ui;
}

void ui_io_free(struct ui_io *ui)
{
    ui_panel_free(ui->panel);
    ui_label_free(&ui->required);

    for (size_t i = 0; i < ui_io_cmds_max; ++i) {
        struct ui_io_cmd *cmd = ui->cmds + i;
        ui_button_free(&cmd->name);
        ui_button_free(&cmd->help);
        ui_button_free(&cmd->exec);

        for (size_t j = 0; j < cmd->len; ++j) {
            struct ui_io_arg *arg = cmd->args + j;
            ui_label_free(&arg->name);
            ui_input_free(&arg->val);
        }
    }

    free(ui);
}

static void ui_io_update(struct ui_io *ui)
{
    const struct im_config *config = im_config_assert(im_id_item(ui->id));

    ui->len = config->io.len;
    assert(ui->len <= ui_io_cmds_max);

    for (size_t i = 0; i < ui->len; ++i) {
        const struct io_cmd *cfg = config->io.list + i;
        struct ui_io_cmd *cmd = ui->cmds + i;

        cmd->io = cfg->io;
        ui_str_set_atom(&cmd->name.str, cmd->io);

        cmd->len = cfg->len;
        assert(cmd->len <= ui_io_args_max);

        cmd->name.s = cmd->io == ui->open ?
            ui_st.button.list.open : ui_st.button.list.close;

        for (size_t j = 0; j < cmd->len; ++j) {
            const struct io_param *param = cfg->params + j;
            struct ui_io_arg *arg = cmd->args + j;

            arg->required = param->required;
            ui_str_setc(&arg->name.str, param->name);
            ui_input_clear(&arg->val);
        }
    }
}

static bool ui_io_event_user(struct ui_io *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_panel_hide(ui->panel);
        ui->id = 0;
        ui->star = coord_nil();
        ui->len = 0;
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(ui->panel)) return false;
        if (coord_is_nil(ui->star)) return false;
        ui->loading = !proxy_chunk(render.proxy, ui->star);
        return false;
    }

    case EV_IO_TOGGLE: {
        if (ui_panel_is_visible(ui->panel))
            ui_panel_hide(ui->panel);
        else ui_panel_show(ui->panel);
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
            ui_panel_hide(ui->panel);
            ui->id = 0;
        }
        return false;
    }

    case EV_MAN_GOTO:
    case EV_MAN_TOGGLE:
    case EV_WORKER_TOGGLE:
    case EV_ENERGY_TOGGLE:
    case EV_STAR_CLEAR:
    case EV_ITEM_CLEAR: {
        ui_panel_hide(ui->panel);
        return false;
    }

    default: { return false; }
    }
}

static void ui_io_help(struct ui_io *ui, struct ui_io_cmd *cmd)
{
    char path[man_path_max] = {0};
    size_t len = snprintf(path, sizeof(path),
            "/items/%s/io/%.*s",
            item_str_c(im_id_item(ui->id)),
            (unsigned) cmd->name.str.len, cmd->name.str.str);

    struct link link = man_link(path, len);
    if (link_is_nil(link)) {
        render_log(st_error, "unable to open link to '%s'", path);
        return;
    }

    render_push_event(EV_MAN_GOTO, link_to_u64(link), 0);
}

static void ui_io_exec(struct ui_io *ui, struct ui_io_cmd *cmd)
{
    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    assert(chunk);

    vm_word args[ui_io_args_max];
    memset(args, 0, sizeof(args));

    size_t len = 0;
    for (size_t i = 0; i < cmd->len; ++i, ++len) {
        struct ui_io_arg *arg = cmd->args + i;

        if (!arg->val.buf.len) {
            if (!arg->required) break;
            render_log(st_error, "missing value for required argument '%.*s'",
                    (unsigned) arg->name.str.len, arg->name.str.str);
            return;
        }

        if (!ui_input_eval(&arg->val, args + i)) return;
    }

    im_id dst = ui->id;
    const vm_word *it = args;
    if (cmd->io == io_send) {
        dst = args[0];
        it++; len--;
    }

    proxy_io(render.proxy, cmd->io, dst, it, len);
}

static void ui_io_toggle(struct ui_io *ui, struct ui_io_cmd *cmd)
{
    if (ui->open == cmd->io) {
        ui->open = io_nil;
        cmd->name.s = ui_st.button.list.close;
        return;
    }

    if (ui->open != io_nil) {
        for (size_t i = 0; i < ui->len; ++i) {
            struct ui_io_cmd *other = ui->cmds + i;
            if (ui->open != other->io) continue;
            other->name.s = ui_st.button.list.close;
            break;
        }
    }

    ui->open = cmd->io;
    cmd->name.s = ui_st.button.list.open;
}


bool ui_io_event(struct ui_io *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_io_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(ui->panel, ev))) return ret != ui_skip;

    if (ui->loading) return ui_panel_event_consume(ui->panel, ev);

    for (size_t i = 0; i < ui->len; ++i) {
        struct ui_io_cmd *cmd = ui->cmds + i;

        if ((ret = ui_button_event(&cmd->name, ev))) {
            if (ret != ui_action) return true;
            ui_io_toggle(ui, cmd);
            return true;
        }

        if ((ret = ui_button_event(&cmd->help, ev))) {
            if (ret != ui_action) return true;
            ui_io_help(ui, cmd);
            return true;
        }

        if (ui->open != cmd->io) continue;

        for (size_t j = 0; j < cmd->len; ++j) {
            struct ui_io_arg *arg = cmd->args + j;

            if (!(ret = ui_input_event(&arg->val, ev))) continue;
            if (ret != ui_action) return true;

            struct ui_io_arg *next = arg + 1;
            if (next == cmd->args + cmd->len) ui_io_exec(ui, cmd);
            else render_push_event(EV_FOCUS_INPUT, (uintptr_t) &next->val, 0);

            return true;
        }

        if ((ret = ui_button_event(&cmd->exec, ev))) {
            if (ret != ui_action) return true;
            ui_io_exec(ui, cmd);
            return true;
        }
    }

    return ui_panel_event_consume(ui->panel, ev);
}

void ui_io_render(struct ui_io *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;
    if (ui->loading) return;

    for (size_t i = 0; i < ui->len; ++i) {
        struct ui_io_cmd *cmd = ui->cmds + i;

        ui_layout_dir(&layout, ui_layout_left);
        ui_button_render(&cmd->help, &layout, renderer);

        ui_layout_dir(&layout, ui_layout_right);
        ui_button_render(&cmd->name, &layout, renderer);

        ui_layout_next_row(&layout);

        if (ui->open != cmd->io) continue;
        ui_layout_sep_y(&layout, 2);

        for (size_t j = 0; j < cmd->len; ++j) {
            struct ui_io_arg *arg = cmd->args + j;

            ui_layout_sep_cols(&layout, 2);

            if (!arg->required) ui_layout_sep_col(&layout);
            else ui_label_render(&ui->required, &layout, renderer);

            ui_label_render(&arg->name, &layout, renderer);
            ui_input_render(&arg->val, &layout, renderer);

            ui_layout_next_row(&layout);
        }

        ui_layout_sep_cols(&layout, 2);
        ui_button_render(&cmd->exec, &layout, renderer);

        ui_layout_next_row(&layout);
        ui_layout_sep_row(&layout);
    }
}
