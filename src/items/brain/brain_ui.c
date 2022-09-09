/* brain_ui.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"
#include "game/proxy.h"
#include "render/render.h"


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

struct ui_brain
{
    struct font *font;

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
    struct ui_label ip;
    struct ui_link ip_val;
    struct ui_label flags, flags_val;

    struct ui_label regs, regs_index, regs_val;

    struct ui_scroll scroll;
    struct ui_label stack, stack_index, stack_val;

    size_t state_len;
    struct im_brain state;
};

static void *ui_brain_alloc(struct font *font)
{
    enum { u8_len = 2, u16_len = 4, u32_len = 8, u64_len = 16 };

    size_t stack_len = vm_stack_len(im_brain_stack_max) * sizeof(vm_word);
    struct ui_brain *ui = calloc(1, sizeof(*ui) + stack_len);

    *ui = (struct ui_brain) {
        .font = font,

        .mod = ui_label_new(font, ui_str_c("mod: ")),
        .mod_val = ui_link_new(font, ui_str_v(symbol_cap)),
        .mod_ver = ui_label_new(font, ui_str_c("ver: ")),
        .mod_ver_val = ui_label_new(font, ui_str_v(u16_len)),
        .mod_fault = ui_label_new(font, ui_str_c("fault: ")),
        .mod_fault_val = ui_label_new(font, ui_str_v(8)),

        .debug = ui_label_new(font, ui_str_c("debug: ")),
        .debug_val = ui_label_new(font, ui_str_v(8)),
        .breakpoint = ui_label_new(font, ui_str_c("break: ")),
        .breakpoint_val = ui_link_new(font, ui_str_v(8)),

        .msg = ui_label_new(font, ui_str_c("msg: ")),
        .msg_len = ui_label_new(font, ui_str_v(3)),
        .msg_index = ui_label_new(font, ui_str_v(u8_len)),
        .msg_val = ui_label_new(font, ui_str_v(u64_len)),

        .spec = ui_label_new(font, ui_str_c("stack: ")),
        .spec_stack = ui_label_new(font, ui_str_v(u8_len)),
        .spec_speed = ui_label_new(font, ui_str_v(u8_len)),
        .spec_sep = ui_label_new(font, ui_str_c("  speed: ")),

        .io = ui_label_new(font, ui_str_c("io:   ")),
        .io_val = ui_label_new(font, ui_str_v(u8_len)),

        .tsc = ui_label_new(font, ui_str_c("tsc:  ")),
        .tsc_val = ui_label_new(font, ui_str_v(u32_len)),

        .ip = ui_label_new(font, ui_str_c("ip:   ")),
        .ip_val = ui_link_new(font, ui_str_v(u32_len)),

        .flags = ui_label_new(font, ui_str_c("flag: ")),
        .flags_val = ui_label_new(font, ui_str_c("XX ")),

        .regs = ui_label_new(font, ui_str_c("registers: ")),
        .regs_index = ui_label_new(font, ui_str_v(u8_len)),
        .regs_val = ui_label_new(font, ui_str_v(u64_len)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .stack = ui_label_new(font, ui_str_c("stack: ")),
        .stack_index = ui_label_new(font, ui_str_v(u8_len)),
        .stack_val = ui_label_new(font, ui_str_v(u64_len)),

        .state_len = sizeof(ui->state) + stack_len,
    };

    ui->msg_index.fg = ui->regs_index.fg = ui->stack_index.fg = rgba_gray(0x88);
    ui->msg_index.bg = ui->regs_index.bg = ui->stack_index.bg = rgba_gray_a(0x44, 0x88);

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
    ui_link_free(&ui->ip_val);

    ui_label_free(&ui->flags);
    ui_label_free(&ui->flags_val);

    ui_label_free(&ui->regs);
    ui_label_free(&ui->regs_index);
    ui_label_free(&ui->regs_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->stack);
    ui_label_free(&ui->stack_index);
    ui_label_free(&ui->stack_val);

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
        ui->mod_val.fg = rgba_gray(0x33);
        ui_str_setc(&ui->mod_val.str, "nil");

        ui->mod_ver_val.fg = rgba_gray(0x33);
        ui_str_setc(&ui->mod_ver_val.str, "nil");
    }
    else {
        struct symbol mod = {0};
        proxy_mod_name(render.proxy, mod_major(state->mod_id), &mod);

        ui->mod_val.fg = rgba_white();
        ui_str_set_symbol(&ui->mod_val.str, &mod);

        ui->mod_ver_val.fg = rgba_white();
        ui_str_set_hex(&ui->mod_ver_val.str, mod_version(state->mod_id));
    }

    if (state->fault) {
        ui_str_setc(&ui->mod_fault_val.str, "true");
        ui->mod_fault_val.fg = rgba_red();
    }
    else {
        ui_str_setc(&ui->mod_fault_val.str, "nil");
        ui->mod_fault_val.fg = rgba_gray(0x33);
    }

    if (state->debug) {
        ui_str_setc(&ui->debug_val.str, "attached");
        ui->debug_val.fg = rgba_green();

        if (state->mod_id && (!old_debug || old_ip != state->vm.ip))
            render_push_event(EV_MOD_SELECT, state->mod_id, state->vm.ip);
    }
    else {
        ui_str_setc(&ui->debug_val.str, "detached");
        ui->debug_val.fg = rgba_gray(0x88);
    }

    if (state->breakpoint == IP_NIL) ui_str_setc(&ui->breakpoint_val.str, "nil");
    else ui_str_set_hex(&ui->breakpoint_val.str, state->breakpoint);

    if (!state->msg.len) ui_str_setc(&ui->msg_len.str, "nil");
    else ui_str_set_u64(&ui->msg_len.str, state->msg.len);

    ui_str_set_hex(&ui->spec_stack.str, state->vm.specs.stack);
    ui_str_set_hex(&ui->spec_speed.str, state->vm.specs.speed);
    ui_str_set_hex(&ui->io_val.str, state->vm.io);
    ui_str_set_hex(&ui->tsc_val.str, state->vm.tsc);
    ui_str_set_hex(&ui->ip_val.str, state->vm.ip);
    ui_scroll_update(&ui->scroll, state->vm.sp);
}

static bool ui_brain_event(void *_ui, const SDL_Event *ev)
{
    struct ui_brain *ui = _ui;
    const struct im_brain *state = &ui->state;

    enum ui_ret ret = ui_nil;

    if ((ret = ui_link_event(&ui->breakpoint_val, ev))) {
        if (state->mod_id && state->breakpoint != IP_NIL)
            render_push_event(EV_MOD_SELECT, state->mod_id, state->breakpoint);
        return ret == ui_consume;
    }

    if ((ret = ui_link_event(&ui->mod_val, ev))) {
        if (state->mod_id)
            render_push_event(EV_MOD_SELECT, state->mod_id, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_link_event(&ui->ip_val, ev))) {
        if (state->mod_id)
            render_push_event(EV_MOD_SELECT, state->mod_id, state->vm.ip);
        return ret == ui_consume;
    }

    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;

    return false;
}

static void ui_brain_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_brain *ui = _ui;
    const struct im_brain *state = &ui->state;

    ui_label_render(&ui->mod, layout, renderer);
    ui_link_render(&ui->mod_val, layout, renderer);
    ui_layout_next_row(layout);
    ui_label_render(&ui->mod_ver, layout, renderer);
    ui_label_render(&ui->mod_ver_val, layout, renderer);
    ui_layout_next_row(layout);
    ui_label_render(&ui->mod_fault, layout, renderer);
    ui_label_render(&ui->mod_fault_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->debug, layout, renderer);
    ui_label_render(&ui->debug_val, layout, renderer);
    ui_layout_next_row(layout);
    ui_label_render(&ui->breakpoint, layout, renderer);
    ui_link_render(&ui->breakpoint_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    { // msg
        ui_label_render(&ui->msg, layout, renderer);
        ui_label_render(&ui->msg_len, layout, renderer);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < im_packet_max; ++i) {
            ui_str_set_u64(&ui->msg_index.str, i);
            ui_label_render(&ui->msg_index, layout, renderer);
            ui_layout_sep_x(layout, ui->font->glyph_w);

            ui_str_set_hex(&ui->msg_val.str, state->msg.data[i]);
            ui_label_render(&ui->msg_val, layout, renderer);
            ui_layout_next_row(layout);
        }
    }

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->spec, layout, renderer);
    ui_label_render(&ui->spec_stack, layout, renderer);
    ui_label_render(&ui->spec_sep, layout, renderer);
    ui_label_render(&ui->spec_speed, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->io, layout, renderer);
    ui_label_render(&ui->io_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->tsc, layout, renderer);
    ui_label_render(&ui->tsc_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->ip, layout, renderer);
    ui_link_render(&ui->ip_val, layout, renderer);
    ui_layout_next_row(layout);

    { // flags
        ui_label_render(&ui->flags, layout, renderer);

        for (size_t i = 0; i < 8; ++i) {
            enum flags flag = 1 << i;
            const char *str = NULL;
            struct rgba color = {0};

            switch (flag) {
            case FLAG_IO:          { str = "IO "; color = rgba_blue(); break; }
            case FLAG_SUSPENDED:   { str = "SU "; color = rgba_blue(); break; }
            case FLAG_FAULT_USER:  { str = "FU "; color = rgba_red(); break; }
            case FLAG_FAULT_REG:   { str = "FR "; color = rgba_red(); break; }
            case FLAG_FAULT_STACK: { str = "FS "; color = rgba_red(); break; }
            case FLAG_FAULT_CODE:  { str = "FC "; color = rgba_red(); break; }
            case FLAG_FAULT_MATH:  { str = "FM "; color = rgba_red(); break; }
            case FLAG_FAULT_IO:    { str = "FI "; color = rgba_red(); break; }
            default:               { str = "   "; break; }
            }

            if (!(state->vm.flags & flag)) color = rgba_gray(0x33);

            ui->flags_val.fg = color;
            ui_str_setc(&ui->flags_val.str, str);
            ui_label_render(&ui->flags_val, layout, renderer);
        }

        ui_layout_next_row(layout);
    }

    ui_layout_sep_y(layout, ui->font->glyph_h);

    { // regs
        ui_label_render(&ui->regs, layout, renderer);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < 4; ++i) {
            ui_str_set_u64(&ui->regs_index.str, i);
            ui_label_render(&ui->regs_index, layout, renderer);
            ui_layout_sep_x(layout, ui->font->glyph_w);

            ui_str_set_hex(&ui->regs_val.str, state->vm.regs[i]);
            ui_label_render(&ui->regs_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        ui_layout_sep_y(layout, ui->font->glyph_h);
    }

    { // stack
        ui_label_render(&ui->stack, layout, renderer);
        ui_layout_next_row(layout);

        struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
        if (ui_layout_is_nil(&inner)) return;

        size_t first = ui_scroll_first(&ui->scroll);
        size_t last = ui_scroll_last(&ui->scroll);

        for (size_t i = first; i < last; ++i) {
            ui_str_set_u64(&ui->stack_index.str, i);
            ui_label_render(&ui->stack_index, &inner, renderer);
            ui_layout_sep_x(&inner, ui->font->glyph_w);

            ui_str_set_hex(&ui->stack_val.str, state->vm.stack[i]);
            ui_label_render(&ui->stack_val, &inner, renderer);
            ui_layout_next_row(&inner);
        }
    }
}
