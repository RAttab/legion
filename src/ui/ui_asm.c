/* ui_asm.c
   Rémi Attab (remi.attab@gmail.com), 02 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_asm_style_default(struct ui_style *s)
{
    s->as = (struct ui_asm_style) {
        .font = font_base,

        .find = { .margin = engine_cell().h / 4 },
        .row = { .fg = s->rgba.index.fg, .bg = s->rgba.index.bg },
        .jmp = { .current = rgba_gray(0xFF), .base = rgba_gray(0x88) },
        .carret = { .fg = s->rgba.carret, .blink = s->carret.blink },

        .bp = {
            .fg = s->rgba.code.bp.fg,
            .bg = s->rgba.code.bp.bg,
            .hover = s->rgba.code.bp.hover,
        },

        .hl = {
            .bg = s->rgba.code.highlight,
            .opaque = 1 * sys_sec, .fade = 300 * sys_msec,
        },

        .fg = s->rgba.fg,
        .keyword = s->rgba.code.keyword,
        .symbol = make_rgba(0xFF, 0xDE, 0xAD, 0xFF), // NavajoWhite

        .current = s->rgba.code.current,
        .select = s->rgba.code.select,
        .highlight = s->rgba.code.highlight,
    };
}

// -----------------------------------------------------------------------------
// asm
// -----------------------------------------------------------------------------

struct ui_asm ui_asm_new(struct dim dim)
{
    const struct ui_asm_style *s = &ui_st.as;

    struct ui_asm ui = {
        .w = make_ui_widget(dim),
        .s = *s,
        .p = ui_panel_current(),

        .scroll = ui_scroll_new(dim, engine_cell()),

        .as = asm_alloc(),

        .bp = { .ip = vm_ip_nil },

        .find = {
            .type = ui_asm_find_nil,
            .op = ui_label_new(ui_str_c("find: ")),
            .value = ui_input_new_s(&ui_st.input.line, 30),
            .exec = ui_button_new_s(&ui_st.button.line, ui_str_c(">")),
            .close = ui_button_new_s(&ui_st.button.line, ui_str_c("x")),
        },
    };

    return ui;
}

void ui_asm_free(struct ui_asm *ui)
{
    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->find.op);
    ui_input_free(&ui->find.value);
    ui_button_free(&ui->find.exec);
    ui_button_free(&ui->find.close);
    asm_free(ui->as);
}

void ui_asm_reset(struct ui_asm *ui)
{
    asm_reset(ui->as);
    ui->mod = nullptr;

    memset(&ui->carret, 0, sizeof(ui->carret));
    ui_scroll_update_rows(&ui->scroll, 0);
}

static void ui_asm_select_begin(struct ui_asm *ui)
{
    ui->select.active = true;
    ui->select.first.row = ui->select.last.row = ui->carret.row;
    ui->select.first.col = ui->select.last.col = ui->carret.col;
}

static void ui_asm_select_move(struct ui_asm *ui)
{
    if (!ui->select.active) return;
    ui->select.last.row = ui->carret.row;
    ui->select.last.col = ui->carret.col;
}

static void ui_asm_select_end(struct ui_asm *ui)
{
    ui->select.active = false;
}

static void ui_asm_select_clear(struct ui_asm *ui)
{
    memset(&ui->select, 0, sizeof(ui->select));
}

void ui_asm_set_mod(struct ui_asm *ui, const struct mod *mod)
{
    ui->mod = mod;
    asm_parse(ui->as, mod);
    ui_scroll_update_rows(&ui->scroll, asm_rows(ui->as));

    memset(&ui->carret, 0, sizeof(ui->carret));
    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
}


void ui_asm_focus(struct ui_asm *ui)
{
    ui_focus_acquire(ui->p, ui);
}

vm_ip ui_asm_ip(struct ui_asm *ui)
{
    if (!ui->mod) return 0;
    return asm_ip(ui->as, ui->carret.row);
}

void ui_asm_goto(struct ui_asm *ui, vm_ip ip)
{
    if (!ui->mod || ip == vm_ip_nil) return;

    uint32_t row = asm_row(ui->as, ip);;

    ui->carret.row = row;
    ui->carret.col = 0;

    ui->hl.row = row;
    ui->hl.ts = sys_now();

    ui_scroll_center(&ui->scroll, ui->carret.row, ui->carret.col);
}

void ui_asm_breakpoint(struct ui_asm *ui, vm_ip ip)
{
    if (ip == ui->bp.ip) return;

    ui->bp.ip = ip;
    ui->bp.row = ip != vm_ip_nil ? asm_row(ui->as, ip) : 0;
}

static void ui_asm_breakpoint_at(struct ui_asm *ui, uint32_t row)
{
    vm_ip ip = vm_ip_nil;
    if (ui->bp.ip == vm_ip_nil || ui->bp.row != row)
        ip = asm_ip(ui->as, row);

    vm_word args = ip != vm_ip_nil ? ip : 0;
    if (!ux_item_io(io_dbg_break, item_brain, &args, 1)) return;

    ui->bp.ip = ip;
    ui->bp.row = row;
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

constexpr uint32_t ui_asm_margin_cols = 1;
constexpr uint32_t ui_asm_row_cols = 8;
constexpr uint32_t ui_asm_jmp_cols = 16;
constexpr uint32_t ui_asm_line_cols = 1 + 6 + 1 + 10 + 1 + 1;
constexpr uint32_t ui_asm_symbol_cols = 26;

constexpr uint32_t ui_asm_line_col =
    ui_asm_row_cols + ui_asm_margin_cols + ui_asm_jmp_cols;

void ui_asm_render(struct ui_asm *ui, struct ui_layout *layout)
{
    struct dim cell = engine_cell();
    ui->w = make_rect_parts(layout->row.pos, ui_layout_remaining(layout));

    if (ui->find.type) {
        ui_layout_dir_vert(layout, ui_layout_down_up);
        struct ui_layout top = ui_layout_split_y(layout,
                ui->find.value.w.h + ui->s.find.margin * 2);

        ui_layout_sep_x(&top, ui->s.find.margin);
        ui_layout_sep_y(&top, ui->s.find.margin);

        ui_label_render(&ui->find.op, &top);
        ui_input_render(&ui->find.value, &top);

        ui_layout_sep_col(&top);
        ui_button_render(&ui->find.exec, &top);

        ui_layout_dir_hori(&top, ui_layout_right_left);
        ui_button_render(&ui->find.close, &top);

        ui_layout_dir_vert(layout, ui_layout_up_down);
    }

    ui->scroll.w.h = ui_layout_inf;
    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout);
    ui->inner = make_rect_parts(inner.base.pos, inner.base.dim);

    if (asm_empty(ui->as) || ui_layout_is_nil(&inner)) return;

    const uint32_t row_first = ui_scroll_first_row(&ui->scroll);
    const uint32_t row_last = ui_scroll_last_row(&ui->scroll);

    struct { bool margin; uint32_t row; } cursor = {0};
    {
        struct rect margin = ui->inner;
        margin.w = (ui_asm_row_cols + ui_asm_margin_cols) * cell.w;
        if ((cursor.margin = ev_mouse_in(margin)))
            cursor.row = ((ev_mouse_pos().y - margin.y) / cell.h) + row_first;
    }

    struct { struct rowcol first, last; } select = {0};
    {
        int cmp = rowcol_cmp(ui->select.first, ui->select.last);
        if (cmp < 0) { select.first = ui->select.first; select.last = ui->select.last; }
        if (cmp > 0) { select.first = ui->select.last; select.last = ui->select.first; }
    }

    enum : render_layer
    {
        layer_select = 0,
        layer_bp,
        layer_current,
        layer_hl,
        layer_text,
        layer_carret,
        layer_len,

        layer_jmp = layer_text,
    };
    const render_layer l = render_layer_push(layer_len);

    for (size_t row = row_first; row < row_last; ++row) {
        asm_it it = asm_at(ui->as, row);

        char line[ui_asm_line_cols] = {0};
        struct asm_line_index index = asm_line_str(it, line, sizeof(line));

        const struct pos base = {
            .x = inner.base.pos.x,
            .y = inner.base.pos.y + ((row - row_first) * cell.h)
        };
        const int16_t x1 = base.x + inner.base.dim.w;

        const int16_t row_x = base.x;
        const int16_t margin_x = row_x + (ui_asm_row_cols * cell.w);
        const int16_t jmp_x = margin_x + (ui_asm_margin_cols * cell.w);
        const int16_t line_x = base.x + (ui_asm_line_col * cell.w);
        const int16_t symbol_x = line_x + (ui_asm_line_cols * cell.w);

        {
            char str[ui_asm_row_cols] = {0};
            str_utox(it->ip, str, sizeof(str));

            render_font_bg(
                    l + layer_text, ui->s.font,
                    ui->s.row.fg, ui->s.row.bg,
                    make_pos(row_x, base.y),
                    str, sizeof(str));
        }

        if (row >= select.first.row && row <= select.last.row) {
            size_t from = 0, to = index.len;
            if (row == select.first.row) from = select.first.col;
            if (row == select.last.row) to = select.last.col;

            render_rect_fill(l + layer_select, ui->s.select, make_rect(
                            line_x + (from * cell.w), base.y,
                            cell.h, (to - from) * cell.w));
        }

        if (ui->bp.ip != vm_ip_nil && ui->bp.row == row) {
            render_rect_fill(l + layer_bp, ui->s.bp.fg,
                    make_rect(margin_x, base.y, cell.w, cell.h));
            render_rect_fill(l + layer_bp, ui->s.bp.bg,
                    make_rect(jmp_x, base.y, x1 - jmp_x, cell.h));
        }
        else if (cursor.margin && cursor.row == row)
            render_rect_fill(l + layer_bp, ui->s.bp.hover,
                    make_rect(margin_x, base.y, cell.w, cell.h));

        if (ui->carret.row == row)
            render_rect_fill(l + layer_current, ui->s.current,
                    make_rect(jmp_x, base.y, x1 - jmp_x, cell.h));

        if (ui->hl.ts && ui->hl.row == row) {
            struct rgba bg = ui->s.hl.bg;
            sys_ts delta = sys_now() - ui->hl.ts;

            if (delta > ui->s.hl.opaque) {
                delta -= ui->s.hl.opaque;
                if (delta > ui->s.hl.fade)
                    memset(&ui->hl, 0, sizeof(ui->hl));
                else bg.a = 0xFF - ((bg.a * delta) / ui->s.hl.fade);
            }

            if (ui->hl.ts) {
                render_rect_fill(l + layer_hl, bg,
                    make_rect(line_x, base.y, ui_asm_line_cols * cell.w, cell.h));
            }
        }

        {
            struct pos pos = { .y = base.y };

            {
                pos.x = line_x + (index.open.pos * cell.w);
                render_font(l + layer_text, ui->s.font, ui->s.fg, pos,
                        line + index.open.pos, index.open.len);
            }

            {
                pos.x = line_x + (index.op.pos * cell.w);
                render_font(l + layer_text, ui->s.font, ui->s.keyword, pos,
                        line + index.op.pos, index.op.len);
            }

            if (index.arg.len) {
                pos.x = line_x + (index.arg.pos * cell.w);
                render_font(l + layer_text, ui->s.font, ui->s.fg, pos,
                        line + index.arg.pos, index.arg.len);
            }

            {
                pos.x = line_x + (index.close.pos * cell.w);
                render_font(l + layer_text, ui->s.font, ui->s.fg, pos,
                        line + index.close.pos, index.close.len);
            }
        }

        if (it->symbol.len) {
            render_font(l + layer_text, ui->s.font, ui->s.symbol,
                    make_pos(symbol_x, base.y),
                    it->symbol.c, it->symbol.len);
        }
    }

    struct asm_jmp_it jmp = asm_jmp_begin(ui->as, row_first, row_last);
    while (asm_jmp_step(ui->as, &jmp)) {
        const uint32_t min = legion_min(jmp.src, jmp.dst);
        const uint32_t max = legion_max(jmp.src, jmp.dst);

        const uint32_t col =
            ui_asm_row_cols + ui_asm_margin_cols +
            (ui_asm_jmp_cols - jmp.layer);

        const unit x1 = inner.base.pos.x + (col * cell.w);
        const unit x0 = x1 - (cell.w / 2);

        const unit ymin = inner.base.pos.y;
        const unit ymax = inner.base.pos.y + inner.base.dim.h;

        const unit y0 = min < row_first ?
            ymin : (unit) (ymin + ((min - row_first) * cell.h) + (cell.h / 2));
        const unit y1 = max >= row_last ?
            ymax : (unit) (ymin + ((max - row_first) * cell.h) + (cell.h / 2));

        struct rgba fg = jmp.src == ui->carret.row || jmp.dst == ui->carret.row ?
            ui->s.jmp.current : ui->s.jmp.base;

        render_line(l + layer_jmp, fg,
                (struct line) { make_pos(x0, y0), make_pos(x0, y1) });

        unit dst = 0;
        struct rect src = { .x = x0, .y = 0, .w = 6, .h = 6 };

        if (min >= row_first)
            min == jmp.src ? (src.y = y0) : (dst = y0);
        if (max < row_last)
            max == jmp.src ? (src.y = y1) : (dst = y1);

        if (src.y) {
            src.x -= src.w / 2;
            src.y -= src.h / 2;
            render_rect_fill(l + layer_jmp, fg, src);
        }
        if (dst) {
            render_line(l + layer_jmp, fg,
                    (struct line) { make_pos(x0, dst), make_pos(x1, dst) });
        }
    }

    do {
        if (ui_focus_element() != ui) break;
        if (ui->carret.row < row_first || ui->carret.row >= row_last) break;
        if (((sys_now() / ui->s.carret.blink) % 2) == 0) break;

        struct rect carret = {
            .x = inner.base.pos.x + ((ui->carret.col + ui_asm_line_col) * cell.w),
            .y = inner.base.pos.y + ((ui->carret.row - row_first) * cell.h),
            .w = cell.w, .h = cell.h,
        };
        render_rect_fill(l + layer_carret, ui->s.carret.fg, carret);
    } while (false);

    render_layer_pop();
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static bool ui_asm_event_click(struct ui_asm *ui)
{
    struct dim cell = engine_cell();
    struct pos cursor = ev_mouse_pos();
    struct rect rect = ui->inner;

    if (!rect_contains(rect, cursor)) {
        ui_focus_release(ui->p, ui);
        return false;
    }
    ui_focus_acquire(ui->p, ui);


    ui->carret.row = (cursor.y - rect.y) / cell.h;
    ui->carret.row += ui_scroll_first_row(&ui->scroll);
    ui->carret.row = legion_min(ui->carret.row, asm_rows(ui->as));

    ui->carret.col = (cursor.x - rect.x) / cell.w;
    if (ui->carret.col >= ui_asm_line_col) {
        ui->carret.col -= ui_asm_line_col;

        size_t line_cols = asm_line_len(asm_at(ui->as, ui->carret.row));
        ui->carret.col = legion_min(ui->carret.col, line_cols);
    }
    else {
        if (ui->carret.col < ui_asm_row_cols + ui_asm_margin_cols)
            ui_asm_breakpoint_at(ui, ui->carret.row);
        ui->carret.col = 0;
    }

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    ui_asm_select_move(ui);

    return true;
}

static void ui_asm_event_move(
        struct ui_asm *ui, enum ev_mods mods, int32_t row, int32_t col)
{
    if (mods == ev_mods_ctrl) {
        if (col > 0) {
            asm_jmp_it it = asm_jmp_from(ui->as, ui->carret.row);
            if (it) ui->carret.row = it->dst;
        }

        if (col < 0) {
            asm_jmp_it it = asm_jmp_to(ui->as, ui->carret.row);
            if (it) ui->carret.row = it->src;
        }
    }

    else {
        size_t row_cap(void) { return asm_line_len(asm_at(ui->as, ui->carret.row)); }

        if (col < 0) {
            if (ui->carret.col) ui->carret.col--;
            else { row = -1; ui->carret.col = row_cap(); }
        }

        if (col > 0) {
            if (ui->carret.col < row_cap()) ui->carret.col++;
            else { row = +1; ui->carret.col = 0; }
        }

        if (row < 0 && ui->carret.row) ui->carret.row--;
        if (row > 0 && ui->carret.row < asm_rows(ui->as) - 1)
            ui->carret.row++;

        if (row) ui->carret.col = legion_min(ui->carret.col, row_cap());
    }

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    ui_asm_select_move(ui);
}

static void ui_asm_event_home(struct ui_asm *ui, enum ev_mods mods)
{
    ui->carret.col = 0;
    if (mods == ev_mods_ctrl) ui->carret.row = 0;

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    ui_asm_select_move(ui);
}

static void ui_asm_event_end(struct ui_asm *ui, enum ev_mods mods)
{
    ui->carret.col = ui_asm_line_cols - 1;
    if (mods == ev_mods_ctrl)
        ui->carret.row = asm_rows(ui->as) - 1;

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    ui_asm_select_move(ui);
}

static void ui_asm_event_page(struct ui_asm *ui, int32_t dir)
{
    if (dir < 0) {
        ui_scroll_page_up(&ui->scroll);
        ui->carret.row = ui_scroll_first_row(&ui->scroll);
    }

    if (dir > 0) {
        ui_scroll_page_down(&ui->scroll);
        ui->carret.row = ui_scroll_last_row(&ui->scroll) - 1;
    }
}

static void ui_asm_event_center(struct ui_asm *ui, enum ev_mods mods)
{
    if (mods == ev_mods_shift) ui->scroll.rows.first = ui->carret.row;
    else ui_scroll_center(&ui->scroll, ui->carret.row, ui->carret.col);
}

static void ui_asm_event_copy(struct ui_asm *ui)
{
    struct rowcol first, last;
    int cmp = rowcol_cmp(ui->select.first, ui->select.last);
    if (!cmp) { ux_log(st_warn, "nothing selected"); return; }
    else if (cmp < 0) { first = ui->select.first; last = ui->select.last; }
    else if (cmp > 0) { first = ui->select.last; last = ui->select.first; }
    ui_asm_select_clear(ui);

    size_t cap = (last.row - first.row) * ui_asm_line_cols + (last.col - first.col);
    char *buffer = malloc(cap); char *it = buffer;

    for (size_t row = first.row; row <= last.row; ++row) {
        char line[ui_asm_line_cols] = {0};
        struct asm_line_index index = asm_line_str(
                asm_at(ui->as, row), line, sizeof(line));

        size_t from = 0, to = index.len + 1;
        if (row == first.row) from = first.col;
        if (row == last.row) to = last.col;

        bool eol = false;
        if (to == index.len + 1) { eol = true; to--; }

        memcpy(it, line + from, to - from); it += to - from;
        if (eol) { *it = '\n'; ++it; }
    }

    ui_clipboard_copy(buffer, it - buffer);
    free(buffer);
}

// Just about everything about this error here is wrong so it has to be a false
// positive.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"

static void ui_asm_event_help(struct ui_asm *ui)
{
    char line[ui_asm_line_cols] = {0};
    struct asm_line_index index = asm_line_str(
            asm_at(ui->as, ui->carret.row), line, sizeof(line));

    str_to_lower_case(line + index.op.pos, index.op.len);
    ux_man_show_slot_path(ux_slot_right, "/asm/%.*s",
            (unsigned) index.op.len, line + index.op.pos);
}

#pragma GCC diagnostic pop

static void ui_asm_event_find(struct ui_asm *ui, enum ui_asm_find_type type)
{
    if (!type) { ui->find.type = type; return; }

    if (type != ui->find.type) {
        ui->find.type = type;
        ui->find.len = 0;

        const char *op =
            type == ui_asm_find_row ? "goto: " :
            type == ui_asm_find_text ? "find: " :
            nullptr;
        assert(op != nullptr);
        ui_str_setc(&ui->find.op.str, op);

        ui_input_clear(&ui->find.value);
        ui_input_focus(&ui->find.value);
        return;
    }

    switch (type)
    {

    case ui_asm_find_row: {
        uint64_t ip = 0;
        if (!ui_input_get_hex(&ui->find.value, &ip)) {
            ux_log(st_error, "invalid line value");
            break;
        }

        ui->carret.col = 0;
        ui->carret.row = asm_row(ui->as, ip);
        ui_scroll_center(&ui->scroll, ui->carret.row, ui->carret.col);

        ui->hl.row = ui->carret.row;
        ui->hl.ts = sys_now();

        ui_asm_event_find(ui, ui_asm_find_nil);
        ui_asm_focus(ui);
        break;
    }

    case ui_asm_find_text: {
        struct rowcol it = ui->carret;
        if (ui->find.len) it.col += ui->find.len;

        const char *value = nullptr;
        ui->find.len = ui_input_get_str(&ui->find.value, &value);
        if (!ui->find.len) { ux_log(st_error, "missing find value"); break; }

        if (!asm_find(ui->as, &it, value, ui->find.len)) {
            ux_log(st_info, "no matches found");
            break;
        }

        ui->carret = it;
        ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
        ui_asm_select_move(ui);

        ui->hl.row = ui->carret.row;
        ui->hl.ts = sys_now();

        ui_asm_focus(ui);
        break;
    }

    case ui_asm_find_nil: default: { assert(false); }
    }
}

static void ui_asm_event_escape(struct ui_asm *ui)
{
    if (ui->find.type)
        ui_asm_event_find(ui, ui_asm_find_nil);

    else if (rowcol_cmp(ui->select.first, ui->select.last))
        ui_asm_select_clear(ui);
}


void ui_asm_event(struct ui_asm *ui)
{
    if (asm_empty(ui->as)) return;

    ui_scroll_event(&ui->scroll);

    if (ui->find.type) {
        if (ui_input_event(&ui->find.value))
            ui_asm_event_find(ui, ui->find.type);
        if (ui_button_event(&ui->find.exec))
            ui_asm_event_find(ui, ui->find.type);
        if (ui_button_event(&ui->find.close))
            ui_asm_event_find(ui, ui_asm_find_nil);
    }

    for (auto ev = ev_mouse(); ev; ev = nullptr) {
        if (!ev_button_down(ev_button_left)) continue;
        ui_asm_event_click(ui);
    }

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;

        if (ev->state == ev_state_up)
            ui_asm_select_end(ui);

        if (ev->state == ev_state_down && ui_asm_event_click(ui)) {
            ui_asm_select_begin(ui);
            ev_consume_button(ev);
        }
    }

    for (auto ev = ev_next_key(nullptr); ev; ev = ev_next_key(ev)) {
        if (ui_focus_element() != ui) continue;
        if (ev->state == ev_state_up) continue;
        ev_consume_key(ev);

        if (ev->mods == ev_mods_ctrl) {
            if (ev->state == ev_state_repeat) continue;

            switch (ev->c)
            {
            case ev_key_up: { ui_asm_event_move(ui, ev->mods, -1, 0); break; }
            case ev_key_down: { ui_asm_event_move(ui, ev->mods, 1, 0); break; }
            case ev_key_left: { ui_asm_event_move(ui, ev->mods, 0, -1); break; }
            case ev_key_right: { ui_asm_event_move(ui, ev->mods, 0, 1); break; }

            case ev_key_home: { ui_asm_event_home(ui, ev->mods); break; }
            case ev_key_end: { ui_asm_event_end(ui, ev->mods); break; }

            case ev_key_space: {
                ui->select.active ?
                    ui_asm_select_clear(ui) :
                    ui_asm_select_begin(ui);
                break;
            }

            case 'c': { ui_asm_event_copy(ui); break; }
            case 'h': { ui_asm_event_help(ui); break; }
            case 'l': { ui_asm_event_center(ui, ev->mods); break; }
            case 'g': { ui_asm_event_find(ui, ui_asm_find_row); break; }
            case 'f': { ui_asm_event_find(ui, ui_asm_find_text); break; }

            default: { break; }
            }
        }

        else if (!ev->mods) {
            switch (ev->c)
            {
            case ev_key_up: { ui_asm_event_move(ui, ev->mods, -1, 0); break; }
            case ev_key_down: { ui_asm_event_move(ui, ev->mods, 1, 0); break; }
            case ev_key_left: { ui_asm_event_move(ui, ev->mods, 0, -1); break; }
            case ev_key_right: { ui_asm_event_move(ui, ev->mods, 0, 1); break; }

            case ev_key_home: { ui_asm_event_home(ui, ev->mods); break; }
            case ev_key_end: { ui_asm_event_end(ui, ev->mods); break; }

            case ev_key_page_up:   { ui_asm_event_page(ui, -1); break; }
            case ev_key_page_down: { ui_asm_event_page(ui, +1); break; }

            case ev_key_escape: { ui_asm_event_escape(ui); break; }

            default: { break; }
            }
        }
    }
}
