/* ui_input.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_input_style_default(struct ui_style *s)
{
    s->input.base = (struct ui_input_style) {
        .pad = make_dim(2, 2),
        .font = font_base,
        .fg = s->rgba.fg,
        .bg = s->rgba.bg,
        .border = s->rgba.box.border,
        .carret = { .fg = s->rgba.carret, .blink = s->carret.blink },
    };

    s->input.line = s->input.base;
    s->input.line.pad.h = 0;
}


// -----------------------------------------------------------------------------
// input
// -----------------------------------------------------------------------------

struct ui_input ui_input_new_s(const struct ui_input_style *s, size_t len)
{
    assert(len < ui_input_cap);

    struct ui_input input = {
        .w = make_ui_widget(engine_dim_margin(len, 1, s->pad)),
        .s = *s,
        .p = ui_panel_current(),
    };

    input.view.col = 0;
    input.view.len = len;

    input.buf.c = calloc(ui_input_cap, sizeof(input.buf.c));
    input.buf.len = 0;

    input.carret = 0;

    return input;
}

struct ui_input ui_input_new(size_t len)
{
    return ui_input_new_s(&ui_st.input.base, len);
}

void ui_input_free(struct ui_input *input)
{
    free(input->buf.c);
}

void ui_input_focus(struct ui_input *input)
{
    ui_focus_acquire(input->p, input);
}

static void ui_input_view_update(struct ui_input *input)
{
    if (input->carret < input->view.col)
        input->view.col = input->carret;

    if (input->carret >= input->view.col + input->view.len)
        input->view.col = (input->carret - input->view.len) + 1;
}

void ui_input_clear(struct ui_input *input)
{
    input->buf.len = 0;
    input->carret = 0;
    ui_input_view_update(input);

}
void ui_input_set(struct ui_input *input, const char *str)
{
    input->buf.len = strnlen(str, ui_input_cap);
    memcpy(input->buf.c, str, input->buf.len);

    input->carret = 0;
    ui_input_view_update(input);
}

size_t ui_input_get_str(struct ui_input *input, const char **str)
{
    const char *it = input->buf.c;
    const char *end = it + input->buf.len;
    it += str_skip_spaces(it, end - it);

    *str = it;
    return end - it;
}

bool ui_input_get_u64(struct ui_input *input, uint64_t *ret)
{
    const char *it = input->buf.c;
    const char *end = it + input->buf.len;
    it += str_skip_spaces(it, end - it);

    size_t read = str_atou(it, end - it, ret);
    return read > 0;
}

bool ui_input_get_hex(struct ui_input *input, uint64_t *ret)
{
    const char *it = input->buf.c;
    const char *end = it + input->buf.len;
    it += str_skip_spaces(it, end - it);

    size_t read = str_atox(it, end - it, ret);
    return read > 0;
}

bool ui_input_get_symbol(struct ui_input *input, struct symbol *ret)
{
    if (!input->buf.len) return false;
    return symbol_parse(input->buf.c, input->buf.len, ret) >= 0;
}

bool ui_input_eval(struct ui_input *input, vm_word *ret)
{
    if (!input->buf.len) return false;

    struct lisp_ret eval =
        proxy_eval(input->buf.c, input->buf.len);
    *ret = eval.value;

    if (!eval.ok)
        ux_log(st_error, "Invalid const LISP statement: '%s'", input->buf.c);
    return eval.ok;
}

void ui_input_render(struct ui_input *input, struct ui_layout *layout)
{
    ui_layout_add(layout, &input->w);

    const render_layer layer_bg = render_layer_push(3);
    const render_layer layer_fg = layer_bg + 1;
    const render_layer layer_cursor = layer_bg + 2;

    render_rect_fill(layer_bg, input->s.bg, input->w);
    render_rect_line(layer_fg, input->s.border, input->w);

    struct pos pos = {
        .x = input->w.x + input->s.pad.w,
        .y = input->w.y + input->s.pad.h,
    };

    assert(input->view.col <= input->buf.len);
    const char *it = input->buf.c + input->view.col;
    size_t len = legion_min(input->buf.len - input->view.col, input->view.len);
    render_font(layer_fg, input->s.font, input->s.fg, pos, it, len);

    do {
        if (ui_focus_element() != input) break;
        if (((sys_now() / input->s.carret.blink) % 2) == 0) break;

        struct dim cell = engine_cell();
        struct rect carret = {
            .x = pos.x + (input->carret - input->view.col) * cell.w,
            .y = pos.y,
            .w = cell.w,
            .h = cell.h
        };
        render_rect_fill(layer_cursor, input->s.carret.fg, carret);
    } while (false);

    render_layer_pop();
}

// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

static bool ui_input_event_click(struct ui_input *input)
{
    struct pos cursor = ev_mouse_pos();
    if (!rect_contains(input->w, cursor)) {
        ui_focus_release(input->p, input);
        return false;
    }
    ui_focus_acquire(input->p, input);

    size_t col = (cursor.x - input->w.x) / engine_cell().w;
    input->carret = legion_min(col, input->buf.len);

    return true;
}

static void ui_input_event_move(struct ui_input *input, int hori)
{
    if (hori > 0)
        input->carret = legion_min(input->carret+1, input->buf.len);

    if (hori < 0 && input->carret > 0)
        input->carret--;

    ui_input_view_update(input);
}

static void ui_input_event_ins(struct ui_input *input, char key, enum ev_mods mods)
{
    assert(key != '\n');
    if (input->buf.len == ui_input_cap-1) return;

    size_t col = input->carret;
    memmove(input->buf.c + col + 1, input->buf.c + col, input->buf.len - col);

    if (mods == ev_mods_shift) key = str_keycode_shift(key);
    input->buf.c[col] = key;

    input->buf.len++;
    input->carret++;
    ui_input_view_update(input);
}

static void ui_input_event_copy(struct ui_input *input)
{
    ui_clipboard_copy(input->buf.c, input->buf.len);
}

static void ui_input_event_paste(struct ui_input *input)
{
    input->buf.len = ui_clipboard_paste(input->buf.c, ui_input_cap);
    input->carret = input->buf.len;
    ui_input_view_update(input);
}

static void ui_input_event_delete(struct ui_input *input)
{
    if (!input->buf.len) return;

    size_t col = input->carret;
    if (col == input->buf.len) return;

    memmove(input->buf.c + col, input->buf.c + col+1, input->buf.len - col-1);
    input->buf.len--;
}

static void ui_input_event_backspace(struct ui_input *input)
{
    if (!input->buf.len) return;

    size_t col = input->carret;
    if (!col) return;

    memmove(input->buf.c + col-1, input->buf.c + col, input->buf.len - col);
    input->buf.len--;
    input->carret--;
    ui_input_view_update(input);
}


bool ui_input_event(struct ui_input *input)
{
    bool ret = false;

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        if (ev->state != ev_state_up) continue;

        if (ui_input_event_click(input))
            ev_consume_button(ev);
    }

    for (auto ev = ev_next_key(nullptr); ev; ev = ev_next_key(ev)) {
        if (ui_focus_element() != input) continue;
        if (ev->state == ev_state_up) continue;
        ev_consume_key(ev);

        switch (ev->c)
        {
        case ev_key_return: { ret = true; break; }

        case ev_key_left: { ui_input_event_move(input, -1); break; }
        case ev_key_right: { ui_input_event_move(input, 1); break; }

        case ' '...'~': { // from 32 to 176 on the ascii table.
            if (ev_mods_allows(ev->mods, ev_mods_shift))
                ui_input_event_ins(input, ev->c, ev->mods);

            else if (ev->mods == ev_mods_ctrl) {
                switch (ev->c)
                {
                case 'c': { ui_input_event_copy(input); break; }
                case 'v': { ui_input_event_paste(input); break; }
                default: { break; }
                }
            }
        }

        case ev_key_delete: { ui_input_event_delete(input); break; }
        case ev_key_backspace: { ui_input_event_backspace(input); break; }

        case ev_key_up:
        case ev_key_home: {
            input->carret = 0;
            ui_input_view_update(input);
            break;
        }

        case ev_key_down:
        case ev_key_end: {
            input->carret = input->buf.len;
            ui_input_view_update(input);
            break;
        }

        default: { break; }
        }
    }

    return ret;
}
