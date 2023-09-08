/* asm.c
   Rémi Attab (remi.attab@gmail.com), 02 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/asm.h"
#include "vm/asm.h"
#include "utils/time.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_asm_style_default(struct ui_style *s)
{
    s->as = (struct ui_asm_style) {
        .font = s->font.base,

        .row = { .fg = s->rgba.index.fg, .bg = s->rgba.index.bg },
        .bp = { .fg = s->rgba.code.bp.fg, .bg = s->rgba.code.bp.bg },
        .jmp = { .current = rgba_gray(0xFF), .base = rgba_gray(0x88) },
        .carret = { .fg = s->rgba.carret, .blink = s->carret.blink },

        .hl = {
            .bg = s->rgba.code.highlight,
            .opaque = 1 * ts_sec, .fade = 300 * ts_msec,
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
        .w = ui_widget_new(dim.w, dim.h),
        .s = *s,
        .p = ui_panel_current(),

        .focused = false,
        .scroll = ui_scroll_new(dim, ui_st.font.dim),

        .as = asm_alloc(),

        .bp = { .ip = vm_ip_nil },
    };

    return ui;
}

void ui_asm_free(struct ui_asm *ui)
{
    ui_scroll_free(&ui->scroll);
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
    render_push_event(ev_focus_input, (uintptr_t) ui, 0);
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
    ui->hl.ts = ts_now();

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
    if (!ui_item_io(io_dbg_break, item_brain, &args, 1)) return;

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

void ui_asm_render(
        struct ui_asm *ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui->w.pos = layout->row.pos;
    ui->w.dim = ui_layout_remaining(layout);
    struct dim cell = make_dim(ui->s.font->glyph_w, ui->s.font->glyph_h);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (asm_empty(ui->as) || ui_layout_is_nil(&inner)) return;

    const uint32_t row_first = ui_scroll_first_row(&ui->scroll);
    const uint32_t row_last = ui_scroll_last_row(&ui->scroll);


    struct { struct rowcol first, last; } select = {0};
    {
        int cmp = rowcol_cmp(ui->select.first, ui->select.last);
        if (cmp < 0) { select.first = ui->select.first; select.last = ui->select.last; }
        if (cmp > 0) { select.first = ui->select.last; select.last = ui->select.first; }
    }

    for (size_t row = row_first; row < row_last; ++row) {
        asm_it it = asm_at(ui->as, row);

        char line[ui_asm_line_cols] = {0};
        struct asm_line_index index = asm_line_str(it, line, sizeof(line));

        const SDL_Point base = {
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
            rgba_render(ui->s.row.bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = row_x, .y = base.y, .h = cell.h,
                .w = cell.w * ui_asm_row_cols }));

            char str[ui_asm_row_cols] = {0};
            str_utox(it->ip, str, sizeof(str));

            SDL_Point pos = { .x = row_x, .y = base.y };
            rgba_render(ui->s.row.fg, renderer);
            font_render(ui->s.font, renderer, pos, ui->s.row.fg, str, sizeof(str));
        }

        if (row >= select.first.row && row <= select.last.row) {
            size_t from = 0, to = index.len;
            if (row == select.first.row) from = select.first.col;
            if (row == select.last.row) to = select.last.col;

            rgba_render(ui->s.select, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = line_x + (from * cell.w), .y = base.y, .h = cell.h,
                .w = (to - from) * cell.w }));
        }

        if (ui->bp.ip != vm_ip_nil && ui->bp.row == row) {
            rgba_render(ui->s.bp.fg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = margin_x, .y = base.y, .w = cell.w, .h = cell.h }));

            rgba_render(ui->s.bp.bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = jmp_x, .y = base.y, .w = x1 - jmp_x, .h = cell.h }));
        }

        if (ui->carret.row == row) {
            rgba_render(ui->s.current, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = jmp_x, .y = base.y, .w = x1 - jmp_x, .h = cell.h }));
        }

        if (ui->hl.ts && ui->hl.row == row) {
            struct rgba bg = ui->s.hl.bg;
            time_sys delta = ts_now() - ui->hl.ts;

            if (delta > ui->s.hl.opaque) {
                delta -= ui->s.hl.opaque;
                if (delta > ui->s.hl.fade)
                    memset(&ui->hl, 0, sizeof(ui->hl));
                else bg.a = 0xFF - ((bg.a * delta) / ui->s.hl.fade);
            }

            if (ui->hl.ts) {
                rgba_render(bg, renderer);
                sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                    .x = line_x, .y = base.y, .h = cell.h,
                    .w = ui_asm_line_cols * cell.w }));
            }
        }

        {
            SDL_Point pos = { .x = 0, .y = base.y };

            {
                pos.x = line_x + (index.open.pos * cell.w);
                font_render(ui->s.font, renderer, pos, ui->s.fg,
                        line + index.open.pos, index.open.len);
            }

            {
                pos.x = line_x + (index.op.pos * cell.w);
                font_render(ui->s.font, renderer, pos, ui->s.keyword,
                        line + index.op.pos, index.op.len);
            }

            if (index.arg.len) {
                pos.x = line_x + (index.arg.pos * cell.w);
                font_render(ui->s.font, renderer, pos, ui->s.fg,
                        line + index.arg.pos, index.arg.len);
            }

            {
                pos.x = line_x + (index.close.pos * cell.w);
                font_render(ui->s.font, renderer, pos, ui->s.fg,
                        line + index.close.pos, index.close.len);
            }
        }

        if (it->symbol.len) {
            SDL_Point pos = { .x = symbol_x, .y = base.y };
            font_render(
                    ui->s.font, renderer, pos, ui->s.symbol,
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

        const int16_t x1 = inner.base.pos.x + (col * cell.w);
        const int16_t x0 = x1 - (cell.w / 2);

        const int16_t ymin = inner.base.pos.y;
        const int16_t ymax = inner.base.pos.y + inner.base.dim.h;

        const int16_t y0 = min < row_first ?
            ymin : (int16_t) (ymin + ((min - row_first) * cell.h) + (cell.h / 2));
        const int16_t y1 = max >= row_last ?
            ymax : (int16_t) (ymin + ((max - row_first) * cell.h) + (cell.h / 2));

        struct rgba fg = jmp.src == ui->carret.row || jmp.dst == ui->carret.row ?
            ui->s.jmp.current : ui->s.jmp.base;
        rgba_render(fg, renderer);

        sdl_err(SDL_RenderDrawLine(renderer, x0, y0, x0, y1));

        int16_t dst = 0;
        SDL_Rect src = { .x = x0, .y = 0, .w = 5, .h = 5 };

        if (min >= row_first)
            min == jmp.src ? (src.y = y0) : (dst = y0);
        if (max < row_last)
            max == jmp.src ? (src.y = y1) : (dst = y1);

        if (src.y) {
            src.x -= src.w / 2;
            src.y -= src.h / 2;
            sdl_err(SDL_RenderFillRect(renderer, &src));
        }
        if (dst) sdl_err(SDL_RenderDrawLine(renderer, x0, dst, x1, dst));
    }

    do {
        if (!ui->focused) break;
        if (ui->carret.row < row_first || ui->carret.row >= row_last) break;
        if (((ts_now() / ui->s.carret.blink) % 2) == 0) break;

        SDL_Rect rect = {
            .x = inner.base.pos.x + ((ui->carret.col + ui_asm_line_col) * cell.w),
            .y = inner.base.pos.y + ((ui->carret.row - row_first) * cell.h),
            .w = cell.w, .h = cell.h
        };
        rgba_render(ui->s.carret.fg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    } while (false);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

enum ui_ret ui_asm_event_click(struct ui_asm *ui)
{
    SDL_Point cursor = ui_cursor_point();
    SDL_Rect rect = ui_widget_rect(&ui->w);

    ui->focused = SDL_PointInRect(&cursor, &rect);
    if (!ui->focused) return ui_nil;

    ui->carret.row = (cursor.y - rect.y) / ui->s.font->glyph_h;
    ui->carret.row += ui_scroll_first_row(&ui->scroll);
    ui->carret.row = legion_min(ui->carret.row, asm_rows(ui->as));

    ui->carret.col = (cursor.x - rect.x) / ui->s.font->glyph_w;
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
    return ui_consume;
}

enum ui_ret ui_asm_event_move(
        struct ui_asm *ui, uint16_t mod, int32_t row, int32_t col)
{
    if (mod & KMOD_CTRL) {
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
    return ui_consume;
}

enum ui_ret ui_asm_event_home(struct ui_asm *ui, uint16_t mod)
{
    ui->carret.col = 0;
    if (mod & KMOD_CTRL) ui->carret.row = 0;

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    ui_asm_select_move(ui);
    return ui_consume;
}

enum ui_ret ui_asm_event_end(struct ui_asm *ui, uint16_t mod)
{
    ui->carret.col = ui_asm_line_cols - 1;
    if (mod & KMOD_CTRL)
        ui->carret.row = asm_rows(ui->as) - 1;

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    ui_asm_select_move(ui);
    return ui_consume;
}

static enum ui_ret ui_asm_event_copy(struct ui_asm *ui)
{
    struct rowcol first, last;
    int cmp = rowcol_cmp(ui->select.first, ui->select.last);
    if (!cmp) { ui_log(st_warn, "nothing selected"); return ui_consume; }
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

    return ui_consume;
}

enum ui_ret ui_asm_event(struct ui_asm *ui, const SDL_Event *ev)
{
    if (render_user_event_is(ev, ev_focus_input))
        ui->focused = (ui == ev->user.data1);

    enum ui_ret ret = ui_nil;
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret;
    if (asm_empty(ui->as)) return ui_nil;

    switch (ev->type)
    {

    case SDL_MOUSEBUTTONDOWN: {
        if (ev->button.button != SDL_BUTTON_LEFT)
            return ui_nil;

        enum ui_ret ret = ui_asm_event_click(ui);
        if (ret) ui_asm_select_begin(ui);
        return ret;
    }

    case SDL_MOUSEMOTION: {
        if (ev->button.button != SDL_BUTTON_LEFT)
            return ui_nil;
        return ui_asm_event_click(ui);
    }

    case SDL_MOUSEBUTTONUP: {
        if (ev->button.button == SDL_BUTTON_LEFT)
            ui_asm_select_end(ui);
        return ui_nil;
    }

    case SDL_KEYDOWN: {
        if (!ui->focused) return ui_nil;

        uint16_t mod = ev->key.keysym.mod;
        SDL_Keycode keysym = ev->key.keysym.sym;
        switch (keysym)
        {
        case SDLK_UP: { return ui_asm_event_move(ui, mod, -1, 0); }
        case SDLK_DOWN: { return ui_asm_event_move(ui, mod, 1, 0); }
        case SDLK_LEFT: { return ui_asm_event_move(ui, mod, 0, -1); }
        case SDLK_RIGHT: { return ui_asm_event_move(ui, mod, 0, 1); }

        case SDLK_HOME: { return ui_asm_event_home(ui, mod); }
        case SDLK_END: { return ui_asm_event_end(ui, mod); }

        case SDLK_SPACE: {
            if (!(mod & KMOD_CTRL)) return ui_nil;
            ui->select.active ? ui_asm_select_clear(ui) : ui_asm_select_begin(ui);
            return ui_consume;
        }

        case 'c': {
            if (!(mod & KMOD_CTRL)) return ui_nil;
            return ui_asm_event_copy(ui);
        }

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }
}
