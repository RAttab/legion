/* code.c
   Rémi Attab (remi.attab@gmail.com), 16 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/code.h"
#include "vm/ast.h"
#include "vm/code.h"
#include "utils/time.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_code_style_default(struct ui_style *s)
{
    s->code = (struct ui_code_style) {
        .font = s->font.base,
        .bold = s->font.bold,

        .row = { .fg = s->rgba.index.fg, .bg = s->rgba.index.bg },
        .bp = { .fg = s->rgba.code.bp.fg, .bg = s->rgba.code.bp.bg },
        .carret = { .fg = s->rgba.carret, .blink = s->carret.blink },

        .hl = {
            .bg = s->rgba.code.highlight,
            .opaque = 1 * ts_sec, .fade = 300 * ts_msec,
        },

        .errors = {
            .fg = make_rgba(0xB2, 0x22, 0x22, 0xFF), // FireBrick
            .bg = make_rgba(0xB2, 0x22, 0x22, 0xAA), // FireBrick
        },

        .fg = s->rgba.fg,
        .comment = s->rgba.code.comment,
        .keyword = s->rgba.code.keyword,
        .atom = make_rgba(0xFF, 0xDE, 0xAD, 0xFF), // NavajoWhite

        .current = s->rgba.code.current,
        .select = s->rgba.code.select,
        .box = s->rgba.box.border,
    };
}

// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct ui_code ui_code_new(struct dim dim)
{
    const struct ui_code_style *s = &ui_st.code;

    const int16_t errors_h = s->font->glyph_h * 8;

    struct ui_code ui = {
        .w = ui_widget_new(dim.w, dim.h),
        .s = *s,
        .p = ui_panel_current(),

        .focused = false,
        .writable = true,
        .scroll = ui_scroll_new(dim, ui_st.font.dim),
        .tooltip = ui_tooltip_new(ui_str_v(ast_log_cap), (SDL_Rect) {0}),
        .errors = ui_list_new(make_dim(ui_layout_inf, errors_h), mod_err_cap + 10),

        .code = code_alloc(),

        .bp = { .ip = vm_ip_nil },
    };

    ui.errors.s.idle.fg = ui.errors.s.hover.fg = ui.errors.s.selected.fg =
        s->errors.fg;

    return ui;
}


void ui_code_free(struct ui_code *ui)
{
    ui_scroll_free(&ui->scroll);
    ui_tooltip_free(&ui->tooltip);
    ui_list_free(&ui->errors);
    code_free(ui->code);
}

void ui_code_reset(struct ui_code *ui)
{
    code_reset(ui->code);
    ui->mod = nullptr;

    memset(&ui->carret, 0, sizeof(ui->carret));

    ui_scroll_update_rows(&ui->scroll, 0);
    ui_scroll_update_cols(&ui->scroll, 0);
    ui_tooltip_hide(&ui->tooltip);
}

static void ui_code_select_begin(struct ui_code *ui)
{
    ui->select.active = true;
    ui->select.first.pos = ui->select.last.pos = ui->carret.pos;
    ui->select.first.row = ui->select.last.row = ui->carret.row;
    ui->select.first.col = ui->select.last.col = ui->carret.col;
}

static void ui_code_select_move(struct ui_code *ui)
{
    if (!ui->select.active) return;
    ui->select.last.pos = ui->carret.pos;
    ui->select.last.row = ui->carret.row;
    ui->select.last.col = ui->carret.col;
}

static void ui_code_select_end(struct ui_code *ui)
{
    ui->select.active = false;
}

static void ui_code_select_clear(struct ui_code *ui)
{
    memset(&ui->select, 0, sizeof(ui->select));
}

enum ui_code_update_flag : uint8_t
{
    ui_code_update_nil = 0,
    ui_code_update_rows = 0x1,
    ui_code_update_edit = 0x2,
    ui_code_update_all = ui_code_update_rows | ui_code_update_edit,
};

static void ui_code_update(struct ui_code *ui, enum ui_code_update_flag flag)
{
    if (flag & ui_code_update_edit) {
        ui->edit = ts_now();
        memset(&ui->hl, 0, sizeof(ui->hl));
    }
    else ui->edit = 0;

    if (flag & ui_code_update_rows) {
        struct rowcol rc = code_rowcol(ui->code);
        ui_scroll_update_rows(&ui->scroll, rc.row);
        ui_scroll_update_cols(&ui->scroll, rc.col + 1);
    }

    {
        struct rowcol rc = code_rowcol_for(ui->code, ui->carret.pos);
        ui_scroll_visible(&ui->scroll, rc.row, rc.col);
        ui->carret.row = rc.row;
        ui->carret.col = rc.col;

        ui_code_select_move(ui);
    }
}

void ui_code_set_mod(struct ui_code *ui, const struct mod *mod)
{
    ui->mod = mod;
    ui_code_set_text(ui, mod->src, mod->src_len);

    ui_list_reset(&ui->errors);
    for (size_t i = 0; i < mod->errs_len; ++i) {
        const struct mod_err *err = mod->errs + i;

        struct rowcol rc = code_rowcol_for(ui->code, err->pos);
        ui_str_setf(ui_list_add(&ui->errors, i + 1),
                "%04u:%04u: %s", rc.row+1, rc.col+1, err->str);
    }
}

void ui_code_set_text(struct ui_code *ui, const char *str, size_t len)
{
    code_set(ui->code, str, len);
    ui_code_update(ui, ui_code_update_rows);
}

void ui_code_focus(struct ui_code *ui)
{
    render_push_event(ev_focus_input, (uintptr_t) ui, 0);
}

bool ui_code_modified(struct ui_code *ui)
{
    return ui->mod->src_hash != code_hash(ui->code);
}

vm_ip ui_code_ip(struct ui_code *ui)
{
    if (!ui->mod) return 0;
    return mod_byte(ui->mod, ui->carret.pos);
}

static void ui_code_highlight(struct ui_code *ui, uint32_t pos, uint32_t len)
{
    ui->carret.pos = pos;

    struct rowcol rc = code_rowcol_for(ui->code, pos);
    ui->hl.len = len;
    ui->hl.row = rc.row;
    ui->hl.col = rc.col;
    ui->hl.ts = ts_now();

    ui_code_update(ui, ui_code_update_nil);
    ui_scroll_center(&ui->scroll, ui->carret.row, ui->carret.col);
}

void ui_code_goto(struct ui_code *ui, vm_ip ip)
{
    if (!ui->mod || ip == vm_ip_nil) return;

    struct mod_index index = mod_index(ui->mod, ip);
    ui_code_highlight(ui, index.pos, index.len);
    ui_code_select_clear(ui);
}

void ui_code_breakpoint(struct ui_code *ui, vm_ip ip)
{
    if (ip == ui->bp.ip) return;

    if (ip == vm_ip_nil) {
        memset(&ui->bp, 0, sizeof(ui->bp));
        ui->bp.ip = ip;
        return;
    }

    struct mod_index index = mod_index(ui->mod, ip);
    struct rowcol rc = code_rowcol_for(ui->code, index.pos);

    ui->bp.ip = ip;
    ui->bp.pos = index.pos;
    ui->bp.row = rc.row;
    ui->bp.col = rc.col;
}

static void ui_code_breakpoint_at(struct ui_code *ui, uint32_t pos)
{
    vm_ip ip = vm_ip_nil;
    if (ui->bp.ip == vm_ip_nil || ui->bp.pos != pos)
        ip = mod_byte(ui->mod, pos);

    vm_word args = ip != vm_ip_nil ? ip : 0;
    if (!ui_item_io(io_dbg_break, item_brain, &args, 1)) return;

    struct rowcol rc = code_rowcol_for(ui->code, pos);

    ui->bp.ip = ip;
    ui->bp.pos = pos;
    ui->bp.row = rc.row;
    ui->bp.col = rc.col;
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

constexpr uint32_t ui_code_row_cols = 4;
constexpr uint32_t ui_code_margin_cols = 1;
constexpr uint32_t ui_code_line_col = ui_code_row_cols + ui_code_margin_cols;

void ui_code_render(
        struct ui_code *ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui->w.pos = layout->row.pos;
    ui->w.dim = ui_layout_remaining(layout);
    struct dim cell = make_dim(ui->s.font->glyph_w, ui->s.font->glyph_h);

    rgba_render(ui->s.box, renderer);
    sdl_err(SDL_RenderDrawLine(renderer,
                    ui->w.pos.x, ui->w.pos.y,
                    ui->w.pos.x,
                    ui->w.pos.y + ui->w.dim.h));

    if (ui->mod->errs_len) {
        ui_layout_dir_vert(layout, ui_layout_down_up);
        struct ui_layout bot = ui_layout_split_y(layout, ui->errors.w.dim.h);
        ui_layout_dir_vert(layout, ui_layout_up_down);

        rgba_render(ui->s.box, renderer);
        sdl_err(SDL_RenderDrawLine(renderer,
                        bot.base.pos.x, bot.base.pos.y,
                        bot.base.pos.x + bot.base.dim.w,
                        bot.base.pos.y));
        ui_list_render(&ui->errors, &bot, renderer);
    }

    // We split the rows from the text so that the scroll bar doesn't show up on
    // the rows.
    struct ui_layout margin = ui_layout_split_x(layout, ui_code_line_col * cell.w);

    ui->scroll.w.dim.h = ui_layout_inf;
    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (code_empty(ui->code) || ui_layout_is_nil(&inner)) return;

    ui_tooltip_hide(&ui->tooltip);

    const uint32_t row_first = ui_scroll_first_row(&ui->scroll);
    const uint32_t row_last = ui_scroll_last_row(&ui->scroll);

    const uint32_t col_first = ui_scroll_first_col(&ui->scroll);
    const uint32_t col_last = ui_scroll_last_col(&ui->scroll);

    for (uint32_t row = 0; row < row_last - row_first; ++row) {
        SDL_Rect rect = {
            .x = margin.base.pos.x,
            .y = margin.base.pos.y + (row * cell.h),
            .w = cell.w * ui_code_row_cols,
            .h = cell.h
        };
        rgba_render(ui->s.row.bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &rect));

        char str[ui_code_row_cols] = {0};
        str_utoa(row_first + row + 1, str, sizeof(str));
        rgba_render(ui->s.row.fg, renderer);
        font_render(
                ui->s.font, renderer,
                (SDL_Point) { .x = rect.x, .y = rect.y },
                ui->s.row.fg,
                str, sizeof(str));

    }

    struct { struct rowcol first, last; bool active; } select = {0};
    {
        select.active = ui->select.first.pos != ui->select.last.pos;
        if (ui->select.first.pos < ui->select.last.pos) {
            select.first = make_rowcol(ui->select.first.row, ui->select.first.col);
            select.last = make_rowcol(ui->select.last.row, ui->select.last.col);
        }
        else {
            select.first = make_rowcol(ui->select.last.row, ui->select.last.col);
            select.last = make_rowcol(ui->select.first.row, ui->select.first.col);
        }
    }

    struct code_it it = code_begin(ui->code, row_first);
    uint32_t cols = code_next_cols(ui->code, &it);
    uint32_t prev = -1U;

    while (code_step(ui->code, &it) && it.row < row_last) {
        const int16_t line_w = inner.base.dim.w;
        const SDL_Point base = {
            .x = inner.base.pos.x,
            .y = inner.base.pos.y + ((it.row - row_first) * cell.h)
        };

        if (it.row == prev) goto tokens;
        prev = it.row;

        if (select.active && it.row >= select.first.row && it.row <= select.last.row) {
            size_t from = 0, to = cols;
            if (it.row == select.first.row) from = select.first.col;
            if (it.row == select.last.row) to = select.last.col;
            if (!(to - from)) to++;

            rgba_render(ui->s.select, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = base.x + (from * cell.w), .y = base.y,
                                .w = (to - from) * cell.w, .h = cell.h }));
        }


        if (ui->bp.ip != vm_ip_nil && ui->bp.row == it.row) {
            rgba_render(ui->s.bp.fg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = base.x - cell.w, .y = base.y,
                                .w = cell.w, .h = cell.h }));

            rgba_render(ui->s.bp.bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = base.x, .y = base.y,
                                .w = line_w, .h = cell.h }));
        }

        if (ui->carret.row == it.row) {
            rgba_render(ui->s.current, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = base.x, .y = base.y,
                                .w = line_w, .h = cell.h }));
        }

        if (ui->hl.ts && ui->hl.row == it.row) {
            struct rgba bg = ui->s.hl.bg;
            time_sys delta = ts_now() - ui->hl.ts;

            if (delta > ui->s.hl.opaque) {
                delta -= ui->s.hl.opaque;
                if (delta > ui->s.hl.fade)
                    memset(&ui->hl, 0, sizeof(ui->hl));
                else bg.a = 0xFF - ((bg.a * delta) / ui->s.hl.fade);
            }

            if (ui->hl.ts) {
                SDL_Rect rect = {
                    .x = base.x + ((ui->hl.col - col_first) * cell.w),
                    .y = base.y,
                    .w = legion_min(ui->hl.len, col_last - ui->hl.col) * cell.w,
                    .h = cell.h,
                };
                rgba_render(bg, renderer);
                sdl_err(SDL_RenderFillRect(renderer, &rect));
            }
        }

      tokens:
        if (it.eol) { cols = code_next_cols(ui->code, &it); continue; }
        if (it.col + it.len <= col_first || it.col >= col_last) continue;

        const ast_it node = code_it_ast_node(&it);
        const ast_log_it log = code_it_ast_log(&it);

        const uint32_t col = legion_max(it.col, col_first);
        const uint32_t first =  col - it.col;
        const uint32_t len = legion_min(it.len, col_last - it.col) - first;
        assert(len <= it.len);

        SDL_Point pos = {.x = base.x + ((col - col_first) * cell.w), .y = base.y };

        if (log) {
            SDL_Rect rect = {
                .x = pos.x, .y = pos.y,
                .w = len * cell.w, .h = cell.h,
            };

            rgba_render(ui->s.errors.bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &rect));

            if (ui_cursor_in(&rect)) {
                ui_tooltip_show(&ui->tooltip);
                ui_str_setc(&ui->tooltip.str, log->msg);
            }
        }

        {
            enum ast_type type = node ? node->type : ast_nil;
            struct rgba fg =
                type == ast_comment ? ui->s.comment :
                type == ast_keyword ? ui->s.keyword :
                type == ast_atom ? ui->s.atom :
                ui->s.fg;

            font_render(ui->s.font, renderer, pos, fg, it.str + first, len);
        }
    }

    do {
        if (!ui->focused) break;

        if (ui->carret.row < row_first || ui->carret.row >= row_last) break;
        if (ui->carret.col < col_first || ui->carret.col >= col_last) break;
        if (((ts_now() / ui->s.carret.blink) % 2) == 0) break;

        SDL_Rect rect = {
            .x = inner.base.pos.x + ((ui->carret.col - col_first) * cell.w),
            .y = inner.base.pos.y + ((ui->carret.row - row_first) * cell.h),
            .w = cell.w, .h = cell.h };
        rgba_render(ui->s.carret.fg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    } while (false);

    ui_tooltip_render(&ui->tooltip, renderer);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

enum ui_ret ui_code_event_click(struct ui_code *ui)
{
    SDL_Point cursor = ui_cursor_point();
    SDL_Rect rect = ui_widget_rect(&ui->w);

    ui->focused = SDL_PointInRect(&cursor, &rect);
    if (!ui->focused) return ui_nil;

    uint32_t row = (cursor.y - rect.y) / ui->s.font->glyph_h;
    row += ui_scroll_first_row(&ui->scroll);

    uint32_t col = (cursor.x - rect.x) / ui->s.font->glyph_w;
    bool in_margins = col < ui_code_line_col;

    col = in_margins ? 0 : col - ui_code_line_col;
    col += ui_scroll_first_col(&ui->scroll);

    ui->carret.pos = code_pos_for(ui->code, row, col);
    if (in_margins) ui_code_breakpoint_at(ui, ui->carret.pos);

    ui_code_select_move(ui);
    ui_code_update(ui, ui_code_update_nil);
    return ui_consume;
}

enum ui_ret ui_code_event_move(
        struct ui_code *ui, uint16_t mod, int32_t row, int32_t col)
{
    (void) mod;

    if (row) ui->carret.pos = code_move_row(ui->code, ui->carret.pos, row);
    if (col) ui->carret.pos = code_move_col(ui->code, ui->carret.pos, col);

    ui_code_select_move(ui);
    ui_code_update(ui, ui_code_update_nil);
    return ui_consume;
}

enum ui_ret ui_code_event_home(struct ui_code *ui, uint16_t mod)
{
    ui->carret.pos = mod & KMOD_CTRL ?
        0 : code_move_home(ui->code, ui->carret.pos);

    ui_code_select_move(ui);
    ui_code_update(ui, ui_code_update_nil);
    return ui_consume;
}

enum ui_ret ui_code_event_end(struct ui_code *ui, uint16_t mod)
{
    ui->carret.pos = mod & KMOD_CTRL ?
        code_len(ui->code) : code_move_end(ui->code, ui->carret.pos);

    ui_code_select_move(ui);
    ui_code_update(ui, ui_code_update_nil);
    return ui_consume;
}

enum ui_ret ui_code_event_put(struct ui_code *ui, char key, uint16_t mod)
{
    if (!ui->writable) return ui_nil;
    if (mod & (KMOD_CTRL | KMOD_ALT)) return ui_nil;
    if (mod & KMOD_SHIFT) key = str_keycode_shift(key);

    code_insert(ui->code, ui->carret.pos, key);
    ui->carret.pos++;

    if (key == '(') code_insert(ui->code, ui->carret.pos, ')');

    ui_code_update(ui, ui_code_update_all);
    return ui_consume;
}

enum ui_ret ui_code_event_del(struct ui_code *ui, uint16_t mod, int32_t inc)
{
    if (!ui->writable) return ui_nil;
    if (mod & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) return ui_nil;

    if (inc < 0 && ui->carret.pos) --ui->carret.pos;
    if (inc > 0 || ui->carret.pos) code_delete(ui->code, ui->carret.pos);

    ui_code_update(ui, ui_code_update_all);
    return ui_consume;
}

enum ui_ret ui_code_event_undo(struct ui_code *ui, uint16_t mod)
{
    if (!ui->writable) return ui_nil;
    if (mod & KMOD_ALT) return ui_nil;

    const bool redo = mod & KMOD_SHIFT;
    const uint32_t pos = redo ? code_redo(ui->code) : code_undo(ui->code);

    if (pos == code_pos_nil) {
        ui_log(st_warn, "nothing to %s", redo ? "redo" : "undo");
        return ui_consume;
    }

    ui->carret.pos = pos;
    ui_code_update(ui, ui_code_update_all);
    return ui_consume;
}

static enum ui_ret ui_code_event_copy(struct ui_code *ui, bool del)
{
    if (ui->select.first.pos == ui->select.last.pos) {
        ui_log(st_warn, "nothing selected");
        return ui_consume;
    }

    uint32_t first = legion_min(ui->select.first.pos, ui->select.last.pos);
    uint32_t last = legion_max(ui->select.first.pos, ui->select.last.pos);
    size_t len = last - first;

    char *buffer = malloc(len);

    size_t written = code_write_range(ui->code, first, last, buffer, len);
    assert(written == len);

    ui_clipboard_copy(buffer, len);
    free(buffer);

    ui_code_select_clear(ui);

    if (!del) return ui_consume;

    ui->carret.pos = first;
    code_delete_range(ui->code, first, last);
    ui_code_update(ui, ui_code_update_all);

    return ui_consume;
}

static enum ui_ret ui_code_event_paste(struct ui_code *ui)
{
    if (!ui_clipboard_len()) {
        ui_log(st_warn, "nothing to copy");
        return ui_consume;
    }

    size_t len = ui_clipboard_len();
    code_insert_range(ui->code, ui->carret.pos, ui_clipboard_str(), len);

    ui->carret.pos += len;
    ui_code_update(ui, ui_code_update_all);

    return ui_consume;
}

enum ui_ret ui_code_event(struct ui_code *ui, const SDL_Event *ev)
{
    switch (render_user_event(ev))
    {

    case ev_focus_input: {
        ui->focused = (ui == ev->user.data1);
        break;
    }

    case ev_frame: {
        if (!ui->edit) break;
        if ((ts_now() - ui->edit) < (500 * ts_msec)) break;
        code_update(ui->code);
        ui->edit = 0;
        break;
    }

    default: { break; }
    }

    enum ui_ret ret = ui_nil;
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret;
    if ((ret = ui_tooltip_event(&ui->tooltip, ev))) return ret;
    if (code_empty(ui->code)) return ui_nil;

    if (ui->mod->errs_len && (ret = ui_list_event(&ui->errors, ev))) {
        if (ret != ui_action || !ui->errors.selected) return ret;
        const struct mod_err *err = ui->mod->errs + (ui->errors.selected - 1);
        ui_code_highlight(ui, err->pos, err->len);
        return ret;
    }

    switch (ev->type)
    {

    case SDL_MOUSEBUTTONDOWN: {
        if (ev->button.button != SDL_BUTTON_LEFT)
            return ui_nil;

        enum ui_ret ret = ui_code_event_click(ui);
        if (ret) ui_code_select_begin(ui);
        return ret;
    }

    case SDL_MOUSEMOTION: {
        if (ev->button.button != SDL_BUTTON_LEFT)
            return ui_nil;
        return ui_code_event_click(ui);
    }

    case SDL_MOUSEBUTTONUP: {
        if (ev->button.button == SDL_BUTTON_LEFT)
            ui_code_select_end(ui);
        return ui_nil;
    }

    case SDL_KEYDOWN: {
        if (!ui->focused) return ui_nil;

        uint16_t mod = ev->key.keysym.mod;
        SDL_Keycode keysym = ev->key.keysym.sym;

        if ((mod & KMOD_CTRL)) {
            switch (keysym)
            {
            case SDLK_UP:    { return ui_code_event_move(ui, mod, -1, 0); }
            case SDLK_DOWN:  { return ui_code_event_move(ui, mod, +1, 0); }
            case SDLK_LEFT:  { return ui_code_event_move(ui, mod, 0, -1); }
            case SDLK_RIGHT: { return ui_code_event_move(ui, mod, 0, +1); }

            case SDLK_SPACE: {
                ui->select.active ? ui_code_select_clear(ui) : ui_code_select_begin(ui);
                return ui_consume;
            }

            case 'z': { return ui_code_event_undo(ui, mod); }

            case 'x': { return ui_code_event_copy(ui, true); }
            case 'c': { return ui_code_event_copy(ui, false); }
            case 'v': { return ui_code_event_paste(ui); }

            default: { return ui_nil; }
            }
        }
        else {
            switch (keysym)
            {
            case SDLK_UP:    { return ui_code_event_move(ui, mod, -1, 0); }
            case SDLK_DOWN:  { return ui_code_event_move(ui, mod, +1, 0); }
            case SDLK_LEFT:  { return ui_code_event_move(ui, mod, 0, -1); }
            case SDLK_RIGHT: { return ui_code_event_move(ui, mod, 0, +1); }

            case SDLK_HOME: { return ui_code_event_home(ui, mod); }
            case SDLK_END:  { return ui_code_event_end(ui, mod); }

            // from 32 to 176 on the ascii table. The uppercase letters are not
            // mapped by SDL so they're just skipped
            case ' '...'~':   { return ui_code_event_put(ui, keysym, mod); }
            case SDLK_RETURN: { return ui_code_event_put(ui, '\n', mod); }

            case SDLK_DELETE:    { return ui_code_event_del(ui, mod, +1); }
            case SDLK_BACKSPACE: { return ui_code_event_del(ui, mod, -1); }

            default: { return ui_nil; }
            }
        }
    }

    default: { return ui_nil; }
    }
}
