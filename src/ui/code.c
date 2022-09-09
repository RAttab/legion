/* code.c
   Rémi Attab (remi.attab@gmail.com), 17 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "vm/mod.h"
#include "utils/str.h"

static struct line *ui_code_view_update(struct ui_code *code);


// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct ui_code ui_code_new(struct dim dim, struct font *font)
{
    struct ui_code code = {
        .w = ui_widget_new(dim.w, dim.h),
        .scroll = ui_scroll_new(dim, font->glyph_h),
        .tooltip = ui_tooltip_new(font, ui_str_v(mod_err_cap), (SDL_Rect) {0}),

        .font = font,
        .focused = false,
        .cols = 1,
        .disassembly = false,
    };

    text_init(&code.text);
    code.tooltip.fg = make_rgba(0xFF, 0x00, 0x00, 0xFF);
    return code;
}


void ui_code_free(struct ui_code *code)
{
    ui_scroll_free(&code->scroll);
    ui_tooltip_free(&code->tooltip);
    text_clear(&code->text);
}

void ui_code_focus(struct ui_code *code)
{
    render_push_event(EV_FOCUS_INPUT, (uintptr_t) code, 0);
}

void ui_code_clear(struct ui_code *code)
{
    code->mod = NULL;
    text_clear(&code->text);
}

vm_ip ui_code_ip(struct ui_code *code)
{
    if (code->disassembly) return code->carret.line->user;
    return mod_byte(code->mod, code->carret.row, code->carret.col);
}

void ui_code_goto(struct ui_code *code, vm_ip ip)
{
    if (code->disassembly) {
        code->carret.row = code->carret.col = 0;
        code->carret.line = code->text.first;

        while (code->carret.line->next && code->carret.line->next->user <= ip) {
            code->carret.line = code->carret.line->next;
            code->carret.row++;
        }

        code->mark.row = code->carret.row;
        code->mark.col = 0;
        code->mark.len = code->carret.line->len;
    }

    else {
        struct mod_index index = mod_index(code->mod, ip);

        code->carret.line = code->text.first;
        for (size_t i = 0; i < index.row && code->carret.line->next; ++i)
            code->carret.line = code->carret.line->next;

        code->carret.row = index.row;
        code->carret.col = index.col;

        code->mark.row = index.row;
        code->mark.col = index.col;
        code->mark.len = index.len;
    }

    code->view.top = code->carret.row;
    code->view.line = code->carret.line;
    for (size_t i = 0; i < 5 && code->view.line->prev; ++i) {
        code->view.line = code->view.line->prev;
        code->view.top--;
    }
    ui_code_view_update(code);
}

static void ui_code_set(struct ui_code *code, vm_ip ip)
{
    assert(code->mod);
    assert(code->text.first);

    ui_scroll_update(&code->scroll, code->text.lines);

    code->carret.line = code->text.first;
    code->carret.row = code->carret.col = 0;

    code->view.line = code->text.first;
    code->view.init = true;

    if (ip) ui_code_goto(code, ip);
    else {
        code->carret.row = code->carret.col = 0;
        code->carret.line = code->text.first;

        code->view.top = code->carret.row;
        code->view.line = code->carret.line;
        ui_code_view_update(code);

        code->mark.row = 0;
        code->mark.col = 0;
        code->mark.len = 0;
    }
}

void ui_code_set_code(struct ui_code *code, const struct mod *mod, vm_ip ip)
{
    code->mod = mod;
    code->disassembly = false;

    text_from_str(&code->text, mod->src, mod->src_len);

    ui_code_set(code, ip);
}

void ui_code_set_disassembly(struct ui_code *code, const struct mod *mod, vm_ip ip)
{
    code->mod = mod;
    code->disassembly = true;

    text_clear(&code->text);
    code->text = mod_disasm(code->mod);

    ui_code_set(code, ip);
}

void ui_code_set_text(struct ui_code *code, const char *text, size_t len)
{
    assert(code->mod);
    code->disassembly = false;

    text_from_str(&code->text, text, len);

    ui_code_set(code, 0);
}

void ui_code_indent(struct ui_code *code)
{
    text_indent(&code->text);
    ui_code_view_update(code);

    code->carret.line = code->view.line;
    for (size_t row = code->view.top; row < code->carret.row; ++row)
        code->carret.line = code->carret.line->next;
    code->carret.col = legion_min(code->carret.col, code->carret.line->len);
}


// -----------------------------------------------------------------------------
// view
// -----------------------------------------------------------------------------

static struct line *ui_code_view_update(struct ui_code *code)
{
    code->scroll.first = code->view.top;
    ui_scroll_update(&code->scroll, code->text.lines);

    code->view.cols = 0;
    code->view.bot = code->view.top;

    size_t visible = code->scroll.visible;
    struct line *line = code->view.line;

    while (visible) {
        size_t rows = u64_ceil_div(line->len, code->cols);
        if (rows >= visible) {
            code->view.cols = legion_min(line->len, visible * code->cols);
            break;
        }

        if (!line->next) {
            code->view.cols = line->len;
            break;
        }

        visible -= rows;
        code->view.bot++;
        line = line->next;
    }

    return line;
}

static void ui_code_view_move(struct ui_code *code, size_t row, bool update_carret)
{
    while (row < code->view.top) {
        assert(code->view.line->prev);
        code->view.line = code->view.line->prev;
        code->view.top--;
    }

    while (row > code->view.top) {
        if (!code->view.line->next) break;
        code->view.line = code->view.line->next;
        code->view.top++;
    }

    struct line *bot = ui_code_view_update(code);

    if (update_carret) {
        if (code->carret.row < code->view.top) {
            code->carret.col = 0;
            code->carret.row = code->view.top;
            code->carret.line = code->view.line;
        }

        if (code->carret.row > code->view.bot ||
                (code->carret.row == code->view.bot && code->carret.col > code->view.cols))
        {
            code->carret.col = code->view.cols;
            code->carret.row = code->view.bot;
            code->carret.line = bot;
        }
    }
}

static void ui_code_view_to_carret(struct ui_code *code)
{
    ui_code_view_update(code);

    if (code->carret.row < code->view.top)
        ui_code_view_move(code, code->carret.row, false);

    while (code->carret.row > code->view.bot ||
            (code->carret.row == code->view.bot && code->carret.col > code->view.cols))
    {
        ui_code_view_move(code, code->view.top+1, false);
    }
}

static void ui_code_view_carret_at(struct ui_code *code, size_t row, size_t col)
{
    code->carret.row = code->view.top;
    code->carret.line = code->view.line;

    while (true) {
        size_t rows = u64_ceil_div(code->carret.line->len, code->cols);
        if (rows > row) {
            code->carret.col = legion_min(row * code->cols + col, code->carret.line->len);
            break;
        }

        if (!row || !code->carret.line->next) {
            code->carret.col = code->carret.line->len;
            break;
        }

        code->carret.line = code->carret.line->next;
        code->carret.row++;
        row -= rows;
    }
}

static bool ui_code_view_cursor(struct ui_code *code, size_t *row, size_t *col)
{
    SDL_Point cursor = render.cursor.point;
    SDL_Rect rect = ui_widget_rect(&code->w);
    if (!sdl_rect_contains(&rect, &cursor)) return false;

    size_t rel_col = (cursor.x - code->w.pos.x) / code->font->glyph_w;
    size_t rel_row = (cursor.y - code->w.pos.y) / code->font->glyph_h;
    rel_col = rel_col < (ui_code_num_len+1) ? 0 : rel_col - (ui_code_num_len+1);
    assert(rel_row < code->scroll.visible);
    assert(rel_col < code->cols);

    *row = code->view.top;
    struct line *line = code->view.line;

    while (true) {
        size_t rows = u64_ceil_div(line->len, code->cols);
        if (rows > rel_row) {
            *col = rel_row * code->cols + rel_col;
            return *col < line->len;
        }

        if (!rel_row || !line->next) return false;

        line = line->next;
        rel_row -= rows;
        (*row)++;
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

void ui_code_render(
        struct ui_code *code, struct ui_layout *layout, SDL_Renderer *renderer)
{
    if (!code->mod) return;

    struct ui_layout inner = ui_scroll_render(&code->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    struct font *font = code->font;
    code->w = code->scroll.w;
    code->cols = (inner.base.dim.w / font->glyph_w) - 5;

    // We need both code->cols and code->scroll.visible to initialize the view.
    if (unlikely(code->view.init)) {
        assert(code->scroll.visible);
        ui_code_view_update(code);
        code->view.init = false;
    }

    size_t first = ui_scroll_first(&code->scroll);
    size_t last = first + code->scroll.visible;

    assert(code->view.top == code->scroll.first);
    struct line *line = code->view.line;
    size_t row = code->view.top;
    size_t col = 0;

    struct mod_err *err = code->mod->errs;
    struct mod_err *err_end = err + code->mod->errs_len;

    for (size_t i = first; i < last && line; ++i) {

        // line number
        {
            struct ui_widget w = ui_widget_new(font->glyph_w * 4, font->glyph_h);
            ui_layout_add(&inner, &w);

            rgba_render(rgba_gray_a(0x44, 0x88), renderer);

            SDL_Rect rect = ui_widget_rect(&w);
            sdl_err(SDL_RenderFillRect(renderer, &rect));

            if (!col) {
                char str[4] = {0};
                if (!code->disassembly) str_utoa(row, str, sizeof(str));
                else str_utox(line->user, str, sizeof(str));

                struct rgba fg = rgba_gray(0x88);
                SDL_Point pos = pos_as_point(w.pos);
                font_render(font, renderer, pos, fg, str, sizeof(str));
            }
        }

        ui_layout_sep_x(&inner, code->font->glyph_w);

        // code line
        struct ui_widget w = ui_widget_new(font->glyph_w * code->cols, font->glyph_h);
        ui_layout_add(&inner, &w);

        size_t len = legion_min(line->len - col, code->cols);

        // mark
        if (code->mark.row == row) {
            size_t start = 0, end = code->cols;

            if (!code->disassembly) {
                start = legion_bound(code->mark.col, col, col + len);
                end = legion_bound(code->mark.col + code->mark.len, col, col + len);
            }

            if (start != end) {
                rgba_render(make_rgba(0x00, 0x77, 0x00, 0xFF), renderer);
                sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                    .x = w.pos.x + (start * font->glyph_w),
                                    .y = w.pos.y,
                                    .w = (end - start) * font->glyph_w,
                                    .h = font->glyph_h,
                                }));
            }
        }

        // code text
        {
            struct rgba fg = rgba_white();
            SDL_Point pos = pos_as_point(w.pos);
            font_render(font, renderer, pos, fg, line->c + col, len);
        }

        // err
        while (err < err_end) {
            if (err->row < row) { err++; continue; }
            if (err->row > row) break;

            if (err->col + err->len < col) { err++; continue; }
            if (err->col > col + len) break;

            size_t start = legion_max(err->col, col);
            size_t end = legion_min((size_t) err->col + err->len, col + len);

            rgba_render(make_rgba(0xFF, 0x00, 0x00, 0x77), renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = w.pos.x + (start * font->glyph_w),
                                .y = w.pos.y,
                                .w = (end - start) * font->glyph_w,
                                .h = font->glyph_h,
                            }));
            err++;
        }

        // carret
        if (code->focused && code->carret.blink && code->carret.row == row &&
                (code->carret.col >= col && code->carret.col <= col + len))
        {
            rgba_render(rgba_white(), renderer);
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = w.pos.x + ((code->carret.col - col) * font->glyph_w),
                                .y = w.pos.y,
                                .w = font->glyph_w,
                                .h = font->glyph_h,
                            }));
        }

        // advance
        if (col + len < line->len) col += len;
        else { row++; col = 0; line = line->next; }
        ui_layout_next_row(&inner);
    }

    ui_tooltip_render(&code->tooltip, renderer);
}


// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

static enum ui_ret ui_code_event_click(struct ui_code *code)
{
    SDL_Point cursor = render.cursor.point;
    SDL_Rect rect = ui_widget_rect(&code->w);

    code->focused = sdl_rect_contains(&rect, &cursor);
    if (!code->focused) return ui_nil;

    size_t col = (cursor.x - code->w.pos.x) / code->font->glyph_w;
    size_t row = (cursor.y - code->w.pos.y) / code->font->glyph_h;
    col = col < (ui_code_num_len+1) ? 0 : col - (ui_code_num_len+1);

    ui_code_view_carret_at(code, row, col);

    render_push_event(EV_FOCUS_INPUT, (uintptr_t) code, 0);
    return ui_consume;
}

static enum ui_ret ui_code_event_move(struct ui_code *code, int hori, int vert)
{
    if (hori < 0) {
        if (code->carret.col) code->carret.col--;
        else if (code->carret.line->prev) {
            code->carret.line = code->carret.line->prev;
            code->carret.col = code->carret.line->len;
            code->carret.row--;
        }
    }

    else if (hori > 0) {
        if (code->carret.col < code->carret.line->len) code->carret.col++;
        else if (code->carret.line->next) {
            code->carret.line = code->carret.line->next;
            code->carret.col = 0;
            code->carret.row++;
        }
    }

    else if (vert < 0) {
        if (code->carret.line->prev) {
            code->carret.line = code->carret.line->prev;
            code->carret.col = legion_min(code->carret.col, code->carret.line->len);
            code->carret.row--;
        }
    }

    else if (vert > 0) {
        if (code->carret.line->next) {
            code->carret.line = code->carret.line->next;
            code->carret.col = legion_min(code->carret.col, code->carret.line->len);
            code->carret.row++;
        }
    }

    ui_code_view_to_carret(code);
    return ui_consume;
}

static enum ui_ret ui_code_event_ins(struct ui_code *code, char key, uint16_t mod)
{
    if (code->disassembly) return ui_nil;
    if (mod & (KMOD_CTRL | KMOD_ALT)) return ui_nil;
    if (mod & KMOD_SHIFT) key = str_keycode_shift(key);

    struct line_ret ret =
        line_insert(&code->text, code->carret.line, code->carret.col, key);
    code->carret.col = ret.index;

    if (code->view.line == code->carret.line)
        code->view.line = ret.line;

    if (ret.new) {
        code->carret.line = ret.new;
        code->carret.row++;
    }
    else code->carret.line = ret.line;

    ui_code_view_to_carret(code);
    return ui_consume;
}

static enum ui_ret ui_code_event_delete(struct ui_code *code)
{
    if (code->disassembly) return ui_nil;

    struct line_ret ret =
        line_delete(&code->text, code->carret.line, code->carret.col);
    assert(ret.index == code->carret.col);

    if (code->view.line == code->carret.line)
        code->view.line = ret.line;

    code->carret.line = ret.line;

    ui_code_view_to_carret(code);
    return ui_consume;
}

static enum ui_ret ui_code_event_backspace(struct ui_code *code)
{
    if (code->disassembly) return ui_nil;

    struct line_ret ret =
        line_backspace(&code->text, code->carret.line, code->carret.col);
    code->carret.col = ret.index;

    if (ret.new) {
        if (code->view.line == code->carret.line) {
            code->view.line = ret.new;
            code->view.top--;
        }

        code->carret.line = ret.new;
        code->carret.row--;
    }

    ui_code_view_to_carret(code);
    return ui_consume;
}

static enum ui_ret ui_code_event_user(struct ui_code *code, const SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_TICK: {
        uint64_t ticks = (uintptr_t) ev->user.data1;
        code->carret.blink = (ticks / 20) % 2;
        return ui_nil;
    }

    case EV_FOCUS_INPUT: {
        void *target = ev->user.data1;
        code->focused = target == code;
        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

static enum ui_ret ui_code_event_motion(struct ui_code *code)
{
    code->tooltip.visible = false;
    if (likely(!code->mod->errs_len)) return ui_nil;

    size_t row = 0, col = 0;
    if (!ui_code_view_cursor(code, &row, &col)) return ui_nil;

    for (size_t i = 0; i < code->mod->errs_len; ++i) {
        struct mod_err *err = &code->mod->errs[i];
        if (row < err->row) continue;
        if (row > err->row) break;

        if (col < err->col) continue;
        if (col >= err->col + err->len) break;

        size_t len = strnlen(err->str, mod_err_cap);
        ui_str_setv(&code->tooltip.str, err->str, len);
        code->tooltip.visible = true;
        break;
    }

    return ui_nil;
}


enum ui_ret ui_code_event(struct ui_code *code, const SDL_Event *ev)
{
    if (ev->type == render.event) return ui_code_event_user(code, ev);

    enum ui_ret ret = ui_nil;

    {
        ret = ui_scroll_event(&code->scroll, ev);

        if (code->scroll.first != code->view.top)
            ui_code_view_move(code, code->scroll.first, true);

        assert(code->scroll.first == code->view.top);
        if (ret) return ret;
    }

    if ((ret = ui_tooltip_event(&code->tooltip, ev))) return ret;

    if (!code->mod) return ui_nil;

    switch (ev->type) {

    case SDL_MOUSEMOTION: { return ui_code_event_motion(code); }

    case SDL_MOUSEBUTTONDOWN: { return ui_code_event_click(code); }

    case SDL_KEYDOWN: {
        if (!code->focused) return ui_nil;

        uint16_t mod = ev->key.keysym.mod;
        SDL_Keycode keysym = ev->key.keysym.sym;
        switch (keysym) {

        case SDLK_UP: { return ui_code_event_move(code, 0, -1); }
        case SDLK_DOWN: { return ui_code_event_move(code, 0, 1); }
        case SDLK_LEFT: { return ui_code_event_move(code, -1, 0); }
        case SDLK_RIGHT: { return ui_code_event_move(code, 1, 0); }

        // from 32 to 176 on the ascii table. The uppercase letters are not
        // mapped by SDL so they're just skipped
        case ' '...'~': { return ui_code_event_ins(code, keysym, mod); }
        case SDLK_RETURN: { return ui_code_event_ins(code, '\n', mod); }

        case SDLK_DELETE: { return ui_code_event_delete(code); }
        case SDLK_BACKSPACE: { return ui_code_event_backspace(code); }

        case SDLK_HOME: { code->carret.col = 0; return ui_consume; }
        case SDLK_END: { code->carret.col = code->carret.line->len; return ui_consume; }

        default: { return ui_nil; }
        }
    }

    default: { return ui_nil; }
    }
}
