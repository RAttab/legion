/* ui_brain.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

struct ui_brain
{
    struct ui_label msg, msg_src, msg_index, msg_val;
    struct ui_label mod, mod_val;
    struct ui_label spec, spec_stack, spec_speed, spec_sep;
    struct ui_label flags, flags_val;
    struct ui_label tsc, tsc_val;
    struct ui_label io, io_val, ior, ior_val;
    struct ui_label ip, ip_val;
    struct ui_label regs, regs_index, regs_val;
    struct ui_scroll scroll;
    struct ui_label stack, stack_index, stack_val;
};

static void ui_brain_init(struct ui_brain *ui)
{
    struct font *font = ui_item_font();

    enum { u8_len = 2, u16_len = 4, u32_len = 8, u64_len = 16 };

    *ui = (struct ui_brain) {
        .msg = ui_label_new(font, ui_str_c("msg:  ")),
        .msg_src = ui_label_new(font, ui_str_v(id_str_len)),
        .msg_index = ui_label_new(font, ui_str_v(u8_len)),
        .msg_val = ui_label_new(font, ui_str_v(u64_len)),

        .mod = ui_label_new(font, ui_str_c("mod:  ")),
        .mod_val = ui_label_new(font, ui_str_v(vm_atom_cap)),

        .spec = ui_label_new(font, ui_str_c("spec: ")),
        .spec_stack = ui_label_new(font, ui_str_v(u8_len)),
        .spec_speed = ui_label_new(font, ui_str_v(u8_len)),
        .spec_sep = ui_label_new(font, ui_str_c(" / ")),

        .flags = ui_label_new(font, ui_str_c("font: ")),
        .flags_val = ui_label_new(font, ui_str_c("XX ")),

        .tsc = ui_label_new(font, ui_str_c("tsc:  ")),
        .tsc_val = ui_label_new(font, ui_str_v(u32_len)),

        .io = ui_label_new(font, ui_str_c("io: ")),
        .io_val = ui_label_new(font, ui_str_v(u8_len)),
        .ior = ui_label_new(font, ui_str_c(" ior: ")),
        .ior_val = ui_label_new(font, ui_str_v(u8_len)),

        .ip = ui_label_new(font, ui_str_c("ip:   ")),
        .ip_val = ui_label_new(font, ui_str_v(u32_len)),

        .regs = ui_label_new(font, ui_str_c("reg:  ")),
        .regs_index = ui_label_new(font, ui_str_v(u8_len)),
        .regs_val = ui_label_new(font, ui_str_v(u64_len)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .stack = ui_label_new(font, ui_str_c("stack: ")),
        .stack_index = ui_label_new(font, ui_str_v(u8_len)),
        .stack_val = ui_label_new(font, ui_str_v(u64_len)),
    };

    ui->msg_index.fg = ui->regs_index.fg = ui->stack_index.fg = rgba_gray(0x88);
    ui->msg_index.bg = ui->regs_index.bg = ui->stack_index.bg = rgba_gray_a(0x44, 0x88);
}

static void ui_brain_free(struct ui_brain *ui)
{
    ui_label_free(&ui->msg);
    ui_label_free(&ui->msg_src);
    ui_label_free(&ui->msg_index);
    ui_label_free(&ui->msg_val);

    ui_label_free(&ui->mod);
    ui_label_free(&ui->mod_val);

    ui_label_free(&ui->spec);
    ui_label_free(&ui->spec_stack);
    ui_label_free(&ui->spec_speed);
    ui_label_free(&ui->spec_sep);

    ui_label_free(&ui->flags);
    ui_label_free(&ui->flags_val);

    ui_label_free(&ui->tsc);
    ui_label_free(&ui->tsc_val);

    ui_label_free(&ui->io);
    ui_label_free(&ui->io_val);
    ui_label_free(&ui->ior);
    ui_label_free(&ui->ior_val);

    ui_label_free(&ui->ip);
    ui_label_free(&ui->ip_val);

    ui_label_free(&ui->regs);
    ui_label_free(&ui->regs_index);
    ui_label_free(&ui->regs_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->stack);
    ui_label_free(&ui->stack_index);
    ui_label_free(&ui->stack_val);
}

static void ui_brain_update(struct ui_brain *ui, struct brain *state)
{
    if (!state->msg_src) ui_str_setc(&ui->msg_src.str, "nil");
    else ui_str_set_id(&ui->msg_src.str, state->msg_src);

    if (!state->mod) ui_str_setc(&ui->mod_val.str, "nil");
    else {
        atom_t mod = {0};
        mods_name(state->mod->id, &mod);
        ui_str_setv(&ui->mod_val.str, mod, vm_atom_cap);
    }

    ui_str_set_u64(&ui->spec_stack.str, state->vm.specs.stack);
    ui_str_set_u64(&ui->spec_speed.str, state->vm.specs.speed);
    ui_str_set_u64(&ui->tsc_val.str, state->vm.tsc);
    ui_str_set_u64(&ui->io_val.str, state->vm.io);
    ui_str_set_u64(&ui->ior_val.str, state->vm.ior);
    ui_str_set_u64(&ui->ip_val.str, state->vm.ip);
    ui_scroll_update(&ui->scroll, state->vm.sp);
}

static bool ui_brain_event(
        struct ui_brain *ui, struct brain *state, const SDL_Event *ev)
{
    (void) state;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;
    return false;
}

static void ui_brain_render(
        struct ui_brain *ui, struct brain *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    { // msg
        ui_label_render(&ui->msg, layout, renderer);
        ui_label_render(&ui->msg_src, layout, renderer);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < brain_msg_cap; ++i) {
            ui_str_set_u64(&ui->msg_index.str, i);
            ui_label_render(&ui->msg_index, layout, renderer);
            ui_layout_sep_x(layout, font->glyph_w);

            ui_str_set_u64(&ui->msg_val.str, state->msg[i]);
            ui_label_render(&ui->msg_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        ui_layout_sep_y(layout, font->glyph_h);
    }

    ui_label_render(&ui->mod, layout, renderer);
    ui_label_render(&ui->mod_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->spec, layout, renderer);
    ui_label_render(&ui->spec_stack, layout, renderer);
    ui_label_render(&ui->spec_sep, layout, renderer);
    ui_label_render(&ui->spec_speed, layout, renderer);
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

    ui_label_render(&ui->tsc, layout, renderer);
    ui_label_render(&ui->tsc_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->io, layout, renderer);
    ui_label_render(&ui->io_val, layout, renderer);
    ui_label_render(&ui->ior, layout, renderer);
    ui_label_render(&ui->ior_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->ip, layout, renderer);
    ui_label_render(&ui->ip_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, font->glyph_h);

    { // regs
        ui_label_render(&ui->regs, layout, renderer);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < 4; ++i) {
            ui_str_set_u64(&ui->regs_index.str, i);
            ui_label_render(&ui->regs_index, layout, renderer);
            ui_layout_sep_x(layout, font->glyph_w);

            ui_str_set_u64(&ui->regs_val.str, state->vm.regs[i]);
            ui_label_render(&ui->regs_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        ui_layout_sep_y(layout, font->glyph_h);
    }

    { // stack
        ui_label_render(&ui->stack, layout, renderer);
        ui_layout_next_row(layout);

        struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
        if (ui_layout_is_nil(&inner)) return;

        size_t first = ui_scroll_first(&ui->scroll);
        size_t last = ui_scroll_last(&ui->scroll);

        for (size_t i = first; i < last; ++i) {
            size_t sp = state->vm.sp - i - 1;

            ui_str_set_u64(&ui->stack_index.str, sp);
            ui_label_render(&ui->stack_index, &inner, renderer);
            ui_layout_sep_x(&inner, font->glyph_w);

            ui_str_set_u64(&ui->stack_val.str, state->vm.stack[sp]);
            ui_label_render(&ui->stack_val, &inner, renderer);
            ui_layout_next_row(&inner);
        }
    }
}
