/* ux_io.c
   Rémi Attab (remi.attab@gmail.com), 25 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

constexpr size_t ux_io_cmds_max = 32;
constexpr size_t ux_io_args_max = 5;

struct ux_io_arg
{
    bool required;
    struct ui_label name;
    struct ui_input val;
};

struct ux_io_cmd
{
    enum io io;

    struct ui_button name;
    struct ui_button help;

    size_t len;
    struct ux_io_arg args[ux_io_args_max];

    struct ui_button exec;
};

struct ux_io
{
    im_id id;
    struct coord star;

    enum io open;

    struct ui_label required;

    size_t len;
    struct ux_io_cmd cmds[ux_io_cmds_max];
};

int16_t ux_io_width(void)
{
    return 36 * engine_cell().w;
}

struct ux_io *ux_io_alloc(void)
{
    struct ux_io *ux = mem_alloc_t(ux);
    *ux = (struct ux_io) {
        .open = io_nil,
        .required = ui_label_new_s(&ui_st.label.required, ui_str_c("*")),
    };

    for (size_t i = 0; i < ux_io_cmds_max; ++i) {
        struct ux_io_cmd *cmd = ux->cmds + i;
        *cmd = (struct ux_io_cmd) {
            .name = ui_button_new_s(&ui_st.button.list.close, ui_str_v(symbol_cap)),
            .help = ui_button_new(ui_str_c("?")),
            .exec = ui_button_new(ui_str_c("exec >>")),
        };

        cmd->name.w.w = ui_layout_inf;

        for (size_t j = 0; j < ux_io_args_max; ++j) {
            cmd->args[j] = (struct ux_io_arg) {
                .name = ui_label_new(ui_str_v(10)),
                .val = ui_input_new(22),
            };
        }
    }

    return ux;
}

void ux_io_free(struct ux_io *ux)
{
    ui_label_free(&ux->required);

    for (size_t i = 0; i < ux_io_cmds_max; ++i) {
        struct ux_io_cmd *cmd = ux->cmds + i;
        ui_button_free(&cmd->name);
        ui_button_free(&cmd->help);
        ui_button_free(&cmd->exec);

        for (size_t j = 0; j < ux_io_args_max; ++j) {
            struct ux_io_arg *arg = cmd->args + j;
            ui_label_free(&arg->name);
            ui_input_free(&arg->val);
        }
    }

    mem_free(ux);
}

void ux_io_show(struct ux_io *ux, struct coord star, im_id id)
{
    ux->id = id;
    ux->star = star;
    const struct im_config *config = im_config_assert(im_id_item(ux->id));

    ux->len = config->io.len;
    assert(ux->len <= ux_io_cmds_max);

    for (size_t i = 0; i < ux->len; ++i) {
        const struct io_cmd *cfg = config->io.list + i;
        struct ux_io_cmd *cmd = ux->cmds + i;

        cmd->io = cfg->io;
        ui_str_set_atom(&cmd->name.str, cmd->io);

        cmd->len = cfg->len;
        assert(cmd->len <= ux_io_args_max);

        cmd->name.s = cmd->io == ux->open ?
            ui_st.button.list.open : ui_st.button.list.close;

        for (size_t j = 0; j < cmd->len; ++j) {
            const struct io_param *param = cfg->params + j;
            struct ux_io_arg *arg = cmd->args + j;

            arg->required = param->required;
            ui_str_setc(&arg->name.str, param->name);
            ui_input_clear(&arg->val);
        }
    }
}

static void ux_io_exec(struct ux_io *ux, struct ux_io_cmd *cmd)
{
    struct chunk *chunk = proxy_chunk(ux->star);
    assert(chunk);

    vm_word args[ux_io_args_max];
    memset(args, 0, sizeof(args));

    size_t len = 0;
    for (size_t i = 0; i < cmd->len; ++i, ++len) {
        struct ux_io_arg *arg = cmd->args + i;

        if (!arg->val.buf.len) {
            if (!arg->required) break;
            ux_log(st_error, "missing value for required argument '%.*s'",
                    (unsigned) arg->name.str.len, arg->name.str.str);
            return;
        }

        if (!ui_input_eval(&arg->val, args + i)) return;
    }

    im_id dst = ux->id;
    const vm_word *it = args;
    if (cmd->io == io_send) {
        dst = args[0];
        it++; len--;
    }

    proxy_io(cmd->io, dst, it, len);
}

static void ux_io_toggle(struct ux_io *ux, struct ux_io_cmd *cmd)
{
    if (ux->open == cmd->io) {
        ux->open = io_nil;
        cmd->name.s = ui_st.button.list.close;
        return;
    }

    if (ux->open != io_nil) {
        for (size_t i = 0; i < ux->len; ++i) {
            struct ux_io_cmd *other = ux->cmds + i;
            if (ux->open != other->io) continue;
            other->name.s = ui_st.button.list.close;
            break;
        }
    }

    ux->open = cmd->io;
    cmd->name.s = ui_st.button.list.open;
    if (cmd->len) ui_input_focus(&cmd->args[0].val);
}


void ux_io_event(struct ux_io *ux)
{
    for (size_t i = 0; i < ux->len; ++i) {
        struct ux_io_cmd *cmd = ux->cmds + i;

        if (ui_button_event(&cmd->name))
            ux_io_toggle(ux, cmd);

        if (ui_button_event(&cmd->help))
            ux_man_show_slot_path(ux_slot_left,
                    "/items/%s/io/%.*s",
                    item_str_c(im_id_item(ux->id)),
                    (unsigned) cmd->name.str.len,
                    cmd->name.str.str);

        if (ux->open != cmd->io) continue;

        for (size_t j = 0; j < cmd->len; ++j) {
            struct ux_io_arg *arg = cmd->args + j;
            if (!ui_input_event(&arg->val)) continue;

            struct ux_io_arg *next = arg + 1;
            if (next == cmd->args + cmd->len) ux_io_exec(ux, cmd);
            else ui_input_focus(&next->val);
        }

        if (ui_button_event(&cmd->exec))
            ux_io_exec(ux, cmd);
    }
}

void ux_io_render(struct ux_io *ux, struct ui_layout *layout)
{
    for (size_t i = 0; i < ux->len; ++i) {
        struct ux_io_cmd *cmd = ux->cmds + i;

        ui_layout_dir(layout, ui_layout_right_left);
        ui_button_render(&cmd->help, layout);

        ui_layout_dir(layout, ui_layout_left_right);
        ui_button_render(&cmd->name, layout);

        ui_layout_next_row(layout);

        if (ux->open != cmd->io) continue;
        ui_layout_sep_y(layout, 2);

        for (size_t j = 0; j < cmd->len; ++j) {
            struct ux_io_arg *arg = cmd->args + j;

            ui_layout_sep_cols(layout, 2);

            if (!arg->required) ui_layout_sep_col(layout);
            else ui_label_render(&ux->required, layout);

            ui_label_render(&arg->name, layout);
            ui_input_render(&arg->val, layout);

            ui_layout_next_row(layout);
        }

        ui_layout_sep_cols(layout, 2);
        ui_button_render(&cmd->exec, layout);

        ui_layout_next_row(layout);
        ui_layout_sep_row(layout);
    }
}
