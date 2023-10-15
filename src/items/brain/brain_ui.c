/* brain_ui.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "engine/engine.h"
#include "ux/ui.h"
#include "ui/ui.h"
#include "game/proxy.h"


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

struct ui_brain_frame
{
    mod_id mod; vm_ip ip;
    struct ui_button show;
};

struct ui_brain
{
    struct ui_label mod;
    struct ui_link mod_val;
    struct ui_label mod_ver, mod_ver_val;
    struct ui_label mod_fault, mod_fault_val;

    struct ui_label debug, debug_val;
    struct ui_label breakpoint;
    struct ui_link breakpoint_val;

    struct ui_label msg, msg_len, msg_index, msg_val;

    struct ui_label spec, spec_stack, spec_speed, spec_sep;
    struct ui_label io, io_val;
    struct ui_label tsc, tsc_val;
    struct ui_label ip, ip_val;
    struct ui_button ip_show;
    struct ui_label sbp, sbp_val;
    struct ui_label flags, flags_val;

    struct ui_label regs, regs_index, regs_val;


    struct
    {
        struct ui_label title, index, val;
        struct ui_scroll scroll;

        struct ui_brain_frame list[vm_stack_len(1)];
    } stack;

    size_t state_len;
    struct im_brain state;
};

static void *ui_brain_alloc(void)
{
    enum { u8_len = 2, u16_len = 4, u32_len = 8, u64_len = 16 };

    size_t stack_len = vm_stack_len(im_brain_stack_max) * sizeof(vm_word);
    struct ui_brain *ui = calloc(1, sizeof(*ui) + stack_len);

    *ui = (struct ui_brain) {
        .mod = ui_label_new(ui_str_c("mod: ")),
        .mod_val = ui_link_new(ui_str_v(symbol_cap)),
        .mod_ver = ui_label_new(ui_str_c("ver: ")),
        .mod_ver_val = ui_label_new(ui_str_v(u16_len)),
        .mod_fault = ui_label_new(ui_str_c("fault: ")),
        .mod_fault_val = ui_label_new_s(&ui_st.label.error, ui_str_v(8)),

        .debug = ui_label_new(ui_str_c("debug: ")),
        .debug_val = ui_label_new_s(&ui_st.label.active, ui_str_v(8)),
        .breakpoint = ui_label_new(ui_str_c("break: ")),
        .breakpoint_val = ui_link_new(ui_str_v(8)),

        .msg = ui_label_new(ui_str_c("msg: ")),
        .msg_len = ui_label_new(ui_str_v(3)),
        .msg_index = ui_label_new_s(&ui_st.label.index, ui_str_v(u8_len)),
        .msg_val = ui_label_new(ui_str_v(u64_len)),

        .spec = ui_label_new(ui_str_c("stack: ")),
        .spec_stack = ui_label_new(ui_str_v(u8_len)),
        .spec_speed = ui_label_new(ui_str_v(u8_len)),
        .spec_sep = ui_label_new(ui_str_c("  speed: ")),

        .io = ui_label_new(ui_str_c("io:   ")),
        .io_val = ui_label_new(ui_str_v(u8_len)),

        .tsc = ui_label_new(ui_str_c("tsc:  ")),
        .tsc_val = ui_label_new(ui_str_v(u32_len)),

        .ip = ui_label_new(ui_str_c("ip:   ")),
        .ip_val = ui_label_new(ui_str_v(u32_len)),
        .ip_show = ui_button_new_s(&ui_st.button.line, ui_str_c(">>")),

        .sbp = ui_label_new(ui_str_c("sbp:  ")),
        .sbp_val = ui_label_new(ui_str_v(u8_len)),

        .flags = ui_label_new(ui_str_c("flag: ")),
        .flags_val = ui_label_new(ui_str_c("XX ")),

        .regs = ui_label_new(ui_str_c("registers: ")),
        .regs_index = ui_label_new_s(&ui_st.label.index, ui_str_v(u8_len)),
        .regs_val = ui_label_new(ui_str_v(u64_len)),

        .stack = {
            .title = ui_label_new(ui_str_c("stack: ")),
            .index = ui_label_new_s(&ui_st.label.index, ui_str_v(u8_len)),
            .val = ui_label_new(ui_str_v(u64_len)),
            .scroll = ui_scroll_new(
                    make_dim(ui_layout_inf, ui_layout_inf),
                    engine_cell()),
        },

        .state_len = sizeof(ui->state) + stack_len,
    };

    for (size_t i = 0; i < array_len(ui->stack.list); ++i)
        ui->stack.list[i].show = ui_button_new_s(&ui_st.button.line, ui_str_c(">>"));

    return ui;
}

static void ui_brain_free(void *_ui)
{
    struct ui_brain *ui = _ui;

    ui_label_free(&ui->mod);
    ui_link_free(&ui->mod_val);
    ui_label_free(&ui->mod_ver);
    ui_label_free(&ui->mod_ver_val);
    ui_label_free(&ui->mod_fault);
    ui_label_free(&ui->mod_fault_val);

    ui_label_free(&ui->debug);
    ui_label_free(&ui->debug_val);
    ui_label_free(&ui->breakpoint);
    ui_link_free(&ui->breakpoint_val);

    ui_label_free(&ui->msg);
    ui_label_free(&ui->msg_len);
    ui_label_free(&ui->msg_index);
    ui_label_free(&ui->msg_val);

    ui_label_free(&ui->spec);
    ui_label_free(&ui->spec_stack);
    ui_label_free(&ui->spec_speed);
    ui_label_free(&ui->spec_sep);

    ui_label_free(&ui->io);
    ui_label_free(&ui->io_val);

    ui_label_free(&ui->tsc);
    ui_label_free(&ui->tsc_val);

    ui_label_free(&ui->ip);
    ui_label_free(&ui->ip_val);
    ui_button_free(&ui->ip_show);

    ui_label_free(&ui->sbp);
    ui_label_free(&ui->sbp_val);

    ui_label_free(&ui->flags);
    ui_label_free(&ui->flags_val);

    ui_label_free(&ui->regs);
    ui_label_free(&ui->regs_index);
    ui_label_free(&ui->regs_val);

    ui_scroll_free(&ui->stack.scroll);
    ui_label_free(&ui->stack.title);
    ui_label_free(&ui->stack.index);
    ui_label_free(&ui->stack.val);
    for (size_t i = 0; i < array_len(ui->stack.list); ++i)
        ui_button_free(&ui->stack.list[i].show);

    free(ui);
}

static void ui_brain_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_brain *ui = _ui;
    const struct im_brain *state = &ui->state;

    bool old_debug = state ? state->debug : false;
    vm_ip old_ip = state ? state->vm.ip : 0;

    bool ok = chunk_copy(chunk, id, &ui->state, ui->state_len);
    assert(ok);

    if (!state->mod_id) {
        ui_set_nil(&ui->mod_val);
        ui_set_nil(&ui->mod_ver_val);
    }
    else {
        struct symbol mod = {0};
        proxy_mod_name(mod_major(state->mod_id), &mod);
        ui_str_set_symbol(ui_set(&ui->mod_val), &mod);

        ui_str_set_hex(ui_set(&ui->mod_ver_val), mod_version(state->mod_id));
    }

    if (state->fault) ui_str_setc(ui_set(&ui->mod_fault_val), "true");
    else ui_set_nil(&ui->mod_fault_val);

    if (state->debug) {
        ui_str_setc(ui_set(&ui->debug_val), "attached");
        if (state->mod_id && (!old_debug || old_ip != state->vm.ip))
            ui_mods_show(state->mod_id, state->vm.ip);
    }
    else {
        ui_str_setc(&ui->debug_val.str, "detached");
        ui->debug_val.disabled = true;
    }

    if (state->breakpoint == vm_ip_nil) ui_set_nil(&ui->breakpoint_val);
    else ui_str_set_hex(ui_set(&ui->breakpoint_val), state->breakpoint);

    ui_mods_debug(state->mod_id, state->debug, state->vm.ip, state->breakpoint);

    if (!state->msg.len) ui_set_nil(&ui->msg_len);
    else ui_str_set_u64(ui_set(&ui->msg_len), state->msg.len);

    ui->ip_show.disabled = !state->mod_id;

    ui_str_set_u64(&ui->spec_stack.str, state->vm.specs.stack);
    ui_str_set_u64(&ui->spec_speed.str, state->vm.specs.speed);
    ui_str_set_hex(&ui->io_val.str, state->vm.io);
    ui_str_set_hex(&ui->tsc_val.str, state->vm.tsc);
    ui_str_set_hex(&ui->ip_val.str, state->vm.ip);
    ui_str_set_hex(&ui->sbp_val.str, state->vm.sbp);

    ui_scroll_update_rows(&ui->stack.scroll, state->vm.sp);
    for (size_t i = 0; i < state->vm.sp; ++i)
        ui->stack.list[i].show.disabled = true;

    uint8_t sp = state->vm.sbp;
    while (sp) {
        vm_ip ip = 0; uint8_t sbp = 0; mod_id mod = 0;
        vm_unpack_ret(state->vm.stack[sp - 1], &ip, &sbp, &mod);

        // The compiler generates a swap before the vm_op_ret to setup the
        // return value. This gives a tiny window in which the sbp chain is
        // corrupted. I think it's possible to recover by looking at the op
        // pointed to by ip but for now bailing is easier.
        if (sbp >= sp) break;

        struct ui_brain_frame *frame = ui->stack.list + (sp - 1);
        frame->show.disabled = false;
        frame->ip = ip;
        frame->mod = mod ? mod : ui->state.mod_id;

        sp = sbp;
    }
}

static void ui_brain_event(void *_ui)
{
    struct ui_brain *ui = _ui;
    const struct im_brain *state = &ui->state;

    if (ui_link_event(&ui->breakpoint_val)) {
        if (!state->mod_id) {
            ui_log(st_error,
                    "unable to jump to breakpoint '%x' while no mods are loaded",
                    state->breakpoint);
        }
        else if (state->breakpoint != vm_ip_nil)
            ui_mods_show(state->mod_id, state->breakpoint);
    }

    if (ui_link_event(&ui->mod_val)) {
        if (state->mod_id) ui_mods_show(state->mod_id, 0);
    }

    if (ui_button_event(&ui->ip_show)) {
        if (!state->mod_id) {
            ui_log(st_error,
                    "unable to jump to ip '%x' while no mods are loaded",
                    state->vm.ip);
        }
        else ui_mods_show(state->mod_id, state->vm.ip);
    }

    ui_scroll_event(&ui->stack.scroll);

    for (size_t i = 0; i < ui->state.vm.sp; ++i) {
        struct ui_brain_frame *frame = ui->stack.list + i;
        if (frame->show.disabled) continue;

        if (ui_button_event(&frame->show))
            ui_mods_show(frame->mod, frame->ip);
    }
}

static void ui_brain_render(void *_ui, struct ui_layout *layout)
{
    struct ui_brain *ui = _ui;
    const struct im_brain *state = &ui->state;

    ui_label_render(&ui->mod, layout);
    ui_link_render(&ui->mod_val, layout);
    ui_layout_next_row(layout);
    ui_label_render(&ui->mod_ver, layout);
    ui_label_render(&ui->mod_ver_val, layout);
    ui_layout_next_row(layout);
    ui_label_render(&ui->mod_fault, layout);
    ui_label_render(&ui->mod_fault_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->debug, layout);
    ui_label_render(&ui->debug_val, layout);
    ui_layout_next_row(layout);
    ui_label_render(&ui->breakpoint, layout);
    ui_link_render(&ui->breakpoint_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    { // msg
        ui_label_render(&ui->msg, layout);
        ui_label_render(&ui->msg_len, layout);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < im_packet_max; ++i) {
            ui_str_set_u64(&ui->msg_index.str, i);
            ui_label_render(&ui->msg_index, layout);
            ui_layout_sep_col(layout);

            ui_str_set_hex(&ui->msg_val.str, state->msg.data[i]);
            ui_label_render(&ui->msg_val, layout);
            ui_layout_next_row(layout);
        }
    }

    ui_layout_sep_row(layout);

    ui_label_render(&ui->spec, layout);
    ui_label_render(&ui->spec_stack, layout);
    ui_label_render(&ui->spec_sep, layout);
    ui_label_render(&ui->spec_speed, layout);
    ui_layout_next_row(layout);
    ui_layout_sep_row(layout);

    ui_label_render(&ui->ip, layout);
    ui_label_render(&ui->ip_val, layout);
    ui_layout_sep_col(layout);
    ui_button_render(&ui->ip_show, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->sbp, layout);
    ui_label_render(&ui->sbp_val, layout);
    ui_layout_next_row(layout);
    ui_label_render(&ui->io, layout);
    ui_label_render(&ui->io_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->tsc, layout);
    ui_label_render(&ui->tsc_val, layout);
    ui_layout_next_row(layout);

    { // flags
        ui_label_render(&ui->flags, layout);

        for (size_t i = 1; i < 8; ++i) {
            enum flags flag = 1 << i;
            const char *str = NULL;
            struct rgba color = {0};

            switch (flag) {
            case FLAG_SUSPENDED:   { str = "SU "; color = ui_st.rgba.warn; break; }
            case FLAG_FAULT_USER:  { str = "FU "; color = ui_st.rgba.error; break; }
            case FLAG_FAULT_REG:   { str = "FR "; color = ui_st.rgba.error; break; }
            case FLAG_FAULT_STACK: { str = "FS "; color = ui_st.rgba.error; break; }
            case FLAG_FAULT_CODE:  { str = "FC "; color = ui_st.rgba.error; break; }
            case FLAG_FAULT_MATH:  { str = "FM "; color = ui_st.rgba.error; break; }
            case FLAG_FAULT_IO:    { str = "FI "; color = ui_st.rgba.error; break; }
            case FLAG_IO: // can never show up.
            default:               { str = "   "; break; }
            }

            if (!(state->vm.flags & flag)) color = ui_st.rgba.disabled;

            ui->flags_val.s.fg = color;
            ui_str_setc(&ui->flags_val.str, str);
            ui_label_render(&ui->flags_val, layout);
        }

        ui_layout_next_row(layout);
    }

    ui_layout_sep_row(layout);

    { // regs
        ui_label_render(&ui->regs, layout);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < 4; ++i) {
            ui_str_set_u64(&ui->regs_index.str, i);
            ui_label_render(&ui->regs_index, layout);
            ui_layout_sep_col(layout);

            ui_str_set_hex(&ui->regs_val.str, state->vm.regs[i]);
            ui_label_render(&ui->regs_val, layout);
            ui_layout_next_row(layout);
        }

        ui_layout_sep_row(layout);
    }

    { // stack
        ui_label_render(&ui->stack.title, layout);
        ui_layout_next_row(layout);

        struct ui_layout inner = ui_scroll_render(&ui->stack.scroll, layout);
        if (ui_layout_is_nil(&inner)) return;

        size_t first = ui_scroll_first_row(&ui->stack.scroll);
        size_t last = ui_scroll_last_row(&ui->stack.scroll);

        for (size_t i = first; i < last; ++i) {
            ui_str_set_u64(&ui->stack.index.str, i);
            ui_label_render(&ui->stack.index, &inner);
            ui_layout_sep_col(&inner);

            ui_str_set_hex(&ui->stack.val.str, state->vm.stack[i]);
            ui_label_render(&ui->stack.val, &inner);
            ui_layout_sep_col(&inner);

            struct ui_brain_frame *frame = ui->stack.list + i;
            if (!frame->show.disabled)
                ui_button_render(&frame->show, &inner);

            ui_layout_next_row(&inner);
        }
    }
}
