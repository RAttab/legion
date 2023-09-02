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

    if (ui->bp.ip != vm_ip_nil && ui->bp.row >= row_first && ui->bp.row < row_last) {
        rgba_render(ui->s.bp.fg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
            .x = inner.base.pos.x + (ui_asm_row_cols * cell.w),
            .y = inner.base.pos.y + ((ui->bp.row - row_first) * cell.h),
            .w = cell.w, .h = cell.h,
        }));

        rgba_render(ui->s.bp.bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
            .x = inner.base.pos.x + ((ui_asm_row_cols + ui_asm_margin_cols) * cell.w),
            .y = inner.base.pos.y + ((ui->bp.row - row_first) * cell.h),
            .w = inner.base.dim.w - ((ui_asm_row_cols + ui_asm_margin_cols) * cell.w),
            .h = cell.h,
        }));
    }

    if (ui->carret.row >= row_first && ui->carret.row < row_last) {
        SDL_Rect rect = {
            .x = inner.base.pos.x + ((ui_asm_row_cols + ui_asm_margin_cols) * cell.w),
            .y = inner.base.pos.y + ((ui->carret.row - row_first) * cell.h),
            .w = inner.base.dim.w - (ui_asm_row_cols * cell.w),
            .h = cell.h,
        };
        rgba_render(ui->s.current, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    if (ui->hl.ts && ui->hl.row >= row_first && ui->hl.row < row_last) {
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
                .x = inner.base.pos.x + (ui_asm_line_col * cell.w),
                .y = inner.base.pos.y + ((ui->hl.row - row_first) * cell.h),
                .w = ui_asm_line_cols * cell.w,
                .h = cell.h,
            }));
        }
    }

    asm_it end = asm_at(ui->as, row_last);
    SDL_Point pos = { .x = inner.base.pos.x, .y = inner.base.pos.y };
    for (asm_it it = asm_at(ui->as, row_first); it < end; ++it) {

        {
            rgba_render(ui->s.row.bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = pos.x, .y = pos.y, .w = cell.w * ui_asm_row_cols, .h = cell.h }));

            char str[ui_asm_row_cols] = {0};
            str_utox(it->ip, str, sizeof(str));
            rgba_render(ui->s.row.fg, renderer);
            font_render(ui->s.font, renderer, pos, ui->s.row.fg, str, sizeof(str));

            pos.x += (ui_asm_row_cols + 1) * cell.w;
        }

        pos.x += ui_asm_jmp_cols * cell.w;

        {
            SDL_Point pt = pos;
            size_t len = 0;

            const char *op = vm_op_str(it->op);
            size_t op_len = strnlen(op, 6);

            char arg[10] = {0};
            size_t arg_len = vm_op_arg_fmt(it->arg, it->value, arg, sizeof(arg));

            rgba_render(ui->s.row.fg, renderer);
            font_render(ui->s.font, renderer, pt, ui->s.fg, "(", 1);
            pt.x += cell.w; len++;

            rgba_render(ui->s.keyword, renderer);
            font_render(ui->s.font, renderer, pt, ui->s.keyword, op, op_len);
            pt.x += op_len * cell.w; len += op_len;

            if (arg_len) {
                pt.x += cell.w; len++;

                rgba_render(ui->s.fg, renderer);
                font_render(ui->s.font, renderer, pt, ui->s.fg, arg, arg_len);
                pt.x += arg_len * cell.w; len += arg_len;
            }

            rgba_render(ui->s.row.fg, renderer);
            font_render(ui->s.font, renderer, pt, ui->s.fg, ")", 1);
            len++;

            pos.x += (ui_asm_line_cols + ui_asm_margin_cols) * cell.w;
        }

        if (it->symbol.len) {
            rgba_render(ui->s.symbol, renderer);
            font_render(
                    ui->s.font, renderer, pos, ui->s.symbol,
                    it->symbol.c, it->symbol.len);
        }

        pos.y += cell.h;
        pos.x = inner.base.pos.x;
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
        ui->carret.col = legion_max(ui->carret.col, ui_asm_row_cols);
    }
    else { ui->carret.col = 0; ui_asm_breakpoint_at(ui, ui->carret.row); }

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    return ui_nil;
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
        if (col < 0) {
            if (ui->carret.col) ui->carret.col--;
            else { row = -1; ui->carret.col = ui_asm_line_cols; }
        }

        if (col > 0) {
            if (ui->carret.col < ui_asm_line_cols) ui->carret.col++;
            else { row = +1; ui->carret.col = 0; }
        }

        if (row < 0 && ui->carret.row) ui->carret.row--;
        if (row > 0 && ui->carret.row < asm_rows(ui->as) - 1)
            ui->carret.row++;
    }

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    return ui_consume;
}

enum ui_ret ui_asm_event_home(struct ui_asm *ui, uint16_t mod)
{
    ui->carret.col = 0;
    if (mod & KMOD_CTRL) ui->carret.row = 0;

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
    return ui_consume;
}

enum ui_ret ui_asm_event_end(struct ui_asm *ui, uint16_t mod)
{
    ui->carret.col = ui_asm_line_cols - 1;
    if (mod & KMOD_CTRL)
        ui->carret.row = asm_rows(ui->as) - 1;

    ui_scroll_visible(&ui->scroll, ui->carret.row, ui->carret.col);
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
    case SDL_MOUSEBUTTONUP: { return ui_asm_event_click(ui); }

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

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }
}
