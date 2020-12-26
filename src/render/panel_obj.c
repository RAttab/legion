/* panel_obj.c
   Rémi Attab (remi.attab@gmail.com), 15 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/font.h"
#include "render/panel.h"
#include "render/core.h"
#include "game/item.h"
#include "game/hunk.h"
#include "game/obj.h"
#include "utils/log.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// panel obj
// -----------------------------------------------------------------------------

struct panel_obj_state
{
    struct layout *layout;
    struct ui_scroll scroll;

    id_t id;
    atom_t mod;
    struct coord star;
    struct obj *obj;

    struct {
        struct ui_click mod;
        struct ui_click ip;
    } click;
};

enum
{
    p_obj_id = 0,
    p_obj_id_sep,
    p_obj_target,
    p_obj_target_sep,
    p_obj_cargo,
    p_obj_cargo_grid,
    p_obj_cargo_sep,
    p_obj_docks,
    p_obj_docks_list,
    p_obj_docks_sep,
    p_obj_mod,
    p_obj_mod_sep,
    p_obj_vm,
    p_obj_vm_tsc,
    p_obj_vm_flags,
    p_obj_vm_io,
    p_obj_vm_ipsp,
    p_obj_vm_regs_sep,
    p_obj_vm_regs,
    p_obj_vm_stack_sep,
    p_obj_vm_stack,
    p_obj_len,
};

static const char p_obj_target_str[] = "target:";
static const char p_obj_cargo_str[] = "cargo:";
static const char p_obj_docks_str[] = "docks:";
static const char p_obj_mod_str[] = "mod:";

static const char p_obj_vm_str[] = "vm:";
static const char p_obj_vm_tsc_str[] = "tsc:";
static const char p_obj_vm_flags_str[] = "flags:";
static const char p_obj_vm_io_str[] = "io:";
static const char p_obj_vm_ior_str[] = "ior:";
static const char p_obj_vm_ip_str[] = "ip:";
static const char p_obj_vm_sp_str[] = "sp:";

enum
{
    p_obj_cargo_cols = 5,
    p_obj_cargo_rows = 2,
    p_obj_cargo_size = 25,
    p_obj_docks_max = 5,

    p_obj_vm_u64_len = 16,
    p_obj_vm_u32_len = 8,
    p_obj_vm_u8_len = 2,

    p_obj_vm_flag_len = 3,
    p_obj_vm_flags_len = (
            sizeof(p_obj_vm_flags_str) + p_obj_vm_flag_len * 8), // 'IO SU FR FS ...'

    p_obj_vm_spec_len = 3,
    p_obj_vm_specs_len = p_obj_vm_spec_len * 2 + 1, // '01/02'

    p_obj_vm_io_len = (
            sizeof(p_obj_vm_io_str) + p_obj_vm_u8_len + 1 +
            sizeof(p_obj_vm_ior_str) + p_obj_vm_u8_len),

    p_obj_vm_ipsp_len = (
            sizeof(p_obj_vm_ip_str) + p_obj_vm_u32_len + 1 +
            sizeof(p_obj_vm_sp_str) + p_obj_vm_u8_len),

    p_obj_vm_reg_len = 4, // 'rX: '
    p_obj_vm_reg_rows = 4,

    p_obj_vm_stack_prefix_len = 5, // 'sXX: '
    p_obj_vm_stack_len = p_obj_vm_stack_prefix_len + p_obj_vm_u64_len,
    p_obj_vm_stack_total_len = p_obj_vm_stack_len + ui_scroll_layout_cols,
};


static void panel_obj_render_id(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_obj_id);
    char str[id_str_len];
    id_str(state->id, sizeof(str), str);
    font_render(layout->font, renderer, str, sizeof(str), layout_entry_pos(layout));
}

static void panel_obj_render_target(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_obj_target);
    font_render(
            layout->font, renderer,
            p_obj_target_str, sizeof(p_obj_target_str),
            layout_entry_pos(layout));

    if (state->obj->target) {
        char str[id_str_len];
        id_str(state->obj->target, sizeof(str), str);
        font_render(layout->font, renderer, str, sizeof(str),
                layout_entry_index_pos(layout, 0, sizeof(p_obj_target)));
    }
}

static void panel_obj_render_cargo(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_cargo);
        font_render(
                layout->font, renderer,
                p_obj_cargo_str, sizeof(p_obj_cargo_str),
                layout_entry_pos(layout));
    }

    struct layout_entry *layout = layout_entry(state->layout, p_obj_cargo_grid);

    size_t len = state->obj->cargos;
    assert(len < layout->rows * layout->cols);

    for (size_t i = 0; i < len; ++i) {
        cargo_t cargo = obj_cargo(state->obj)[i];
        SDL_Rect rect = layout_entry_index(layout, i / layout->cols, i % layout->cols);

        { // border
            uint8_t gray = 0x55;
            sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, 0xAA));
            sdl_err(SDL_RenderFillRect(renderer, &rect));
        }

        { // background
            uint8_t gray = 0x33;
            sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, 0xAA));
            sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                .x = rect.x + 1, .y = rect.y + 1,
                                .w = rect.w - 2, .h = rect.h - 2 }));
        }

        if (cargo) {
            // todo: draw icon and count.
        }
    }
}

static void panel_obj_render_docks(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_docks);
        font_render(
                layout->font, renderer,
                p_obj_docks_str, sizeof(p_obj_docks_str),
                layout_entry_pos(layout));
    }

    struct layout_entry *layout = layout_entry(state->layout, p_obj_docks_list);

    size_t len = state->obj->docks;
    assert(len < layout->rows);

    for (size_t row = 0; row < len; ++row) {
        id_t dock = obj_docks(state->obj)[row];
        SDL_Point pos = layout_entry_index_pos(layout, row, 0);

        char prefix[3] = {'0' + row, ':', ' '};
        font_render(layout->font, renderer, prefix, sizeof(prefix), pos);

        if (dock) {
            char str[id_str_len];
            id_str(dock, sizeof(str), str);
            font_render(layout->font, renderer, str, sizeof(str),
                    layout_entry_index_pos(layout, row, sizeof(prefix)));
        }
        else {
            char empty[] = "empty";
            const uint8_t gray = 0x66;
            sdl_err(SDL_SetTextureColorMod(layout->font->tex, gray, gray, gray));
            font_render(layout->font, renderer, empty, sizeof(empty),
                    layout_entry_index_pos(layout, row, sizeof(prefix)));
            font_reset(layout->font);
        }
    }
}

static void panel_obj_render_mod(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_obj_mod);
    font_render(layout->font, renderer, p_obj_mod_str, sizeof(p_obj_mod_str),
            layout_entry_pos(layout));

    if (state->obj->mod) {
        SDL_Point pos = layout_entry_index_pos(layout, 0, sizeof(p_obj_mod_str));
        ui_click_render(&state->click.mod, renderer, pos);
        font_render(layout->font, renderer, state->mod, sizeof(state->mod), pos);
    }
}

static void panel_obj_render_vm(struct panel_obj_state *state, SDL_Renderer *renderer)
{
    const struct vm *vm = obj_vm(state->obj);

    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_vm);
        SDL_Point pos = layout_entry_pos(layout);

        font_render(layout->font, renderer, p_obj_vm_str, sizeof(p_obj_vm_str), pos);

        const size_t len = p_obj_vm_spec_len;

        char val[p_obj_vm_specs_len];
        str_utoa(vm->specs.stack, val, len);
        val[len] = '/';
        str_utoa(vm->specs.speed, val + len + 1, len);

        pos.x = state->layout->bbox.w - (layout->item.w * sizeof(val));
        font_render(layout->font, renderer, val, sizeof(val), pos);
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_vm_tsc);
        font_render(layout->font, renderer, p_obj_vm_tsc_str, sizeof(p_obj_vm_tsc_str),
                layout_entry_pos(layout));

        char val[p_obj_vm_u32_len];
        str_utox(vm->tsc, val, sizeof(val));
        font_render(layout->font, renderer, val, sizeof(val),
                layout_entry_index_pos(layout, 0, sizeof(p_obj_vm_tsc_str)));
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_vm_flags);
        size_t col = 0;

        font_render(layout->font, renderer, p_obj_vm_flags_str, sizeof(p_obj_vm_flags_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(p_obj_vm_flags_str);

        for (size_t i = 0; i < 8; ++i) {
            enum flags flag = 1 << i;

            const char *val = NULL;
            const size_t val_len = 3;

            struct rgb color = {0};
            const struct rgb gray = { .r = 0x33, .g = 0x33, .b = 0x33 };
            const struct rgb red = { .r = 0xCC, .g = 0x00, .b = 0x00 };
            const struct rgb blue = { .r = 0x00, .g = 0x00, .b = 0xCC };

            switch (flag) {
            case FLAG_IO:          { val = "IO"; color = blue; break; }
            case FLAG_SUSPENDED:   { val = "SU"; color = blue; break; }
            case FLAG_FAULT_REG:   { val = "FR"; color = red; break; }
            case FLAG_FAULT_STACK: { val = "FS"; color = red; break; }
            case FLAG_FAULT_CODE:  { val = "FC"; color = red; break; }
            case FLAG_FAULT_MATH:  { val = "FM"; color = red; break; }
            case FLAG_FAULT_IO:    { val = "FI"; color = red; break; }
            default: { val = "  "; break; }
            }

            if (!(vm->flags & flag)) color = gray;

            sdl_err(SDL_SetTextureColorMod(layout->font->tex, color.r, color.g, color.b));
            font_render(layout->font, renderer, val, val_len,
                    layout_entry_index_pos(layout, 0, col));
            col += val_len;
        }
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_vm_io);
        size_t col = 0;

        font_render(layout->font, renderer, p_obj_vm_io_str, sizeof(p_obj_vm_io_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(p_obj_vm_io_str);

        char io[p_obj_vm_u8_len];
        str_utox(vm->io, io, sizeof(io));
        font_render(layout->font, renderer, io, sizeof(io),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(io) + 1;

        font_render(layout->font, renderer, p_obj_vm_ior_str, sizeof(p_obj_vm_ior_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(p_obj_vm_ior_str);

        char ior[p_obj_vm_u8_len];
        str_utox(vm->ior, ior, sizeof(ior));
        font_render(layout->font, renderer, ior, sizeof(ior),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(ior) + 1;
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_vm_ipsp);
        size_t col = 0;

        font_render(layout->font, renderer, p_obj_vm_ip_str, sizeof(p_obj_vm_ip_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(p_obj_vm_ip_str);

        char ip[p_obj_vm_u32_len];
        str_utox(vm->ip, ip, sizeof(ip));
        ui_click_render(&state->click.ip, renderer,
                layout_entry_index_pos(layout, 0, col));
        font_render(layout->font, renderer, ip, sizeof(ip),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(ip) + 1;

        font_render(layout->font, renderer, p_obj_vm_sp_str, sizeof(p_obj_vm_sp_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(p_obj_vm_sp_str);

        char sp[p_obj_vm_u8_len];
        str_utox(vm->sp, sp, sizeof(sp));
        font_render(layout->font, renderer, sp, sizeof(sp),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(sp) + 1;
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_vm_regs);

        for (size_t i = 0; i < p_obj_vm_reg_rows; ++i) {
            char reg[p_obj_vm_reg_len] = { '$', '1'+i, ':', 0};
            font_render(layout->font, renderer, reg, sizeof(reg),
                    layout_entry_index_pos(layout, i, 0));

            char val[p_obj_vm_u64_len];
            str_utox(vm->regs[i], val, sizeof(val));
            font_render(layout->font, renderer, val, sizeof(val),
                    layout_entry_index_pos(layout, i, sizeof(reg)));
        }
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, p_obj_vm_stack);

        const size_t first = state->scroll.first;
        const size_t rows = u64_min(vm->sp, state->scroll.visible);

        for (size_t i = first; i < first + rows; ++i) {
            size_t sp = vm->sp - i - 1;

            char index[p_obj_vm_stack_prefix_len] = {
                's', str_hexchar(sp >> 4), str_hexchar(sp), ':', 0 };
            font_render(layout->font, renderer, index, sizeof(index),
                    layout_entry_index_pos(layout, i - first, 0));

            char val[p_obj_vm_u64_len];
            str_utox(vm->stack[sp], val, sizeof(val));
            font_render(layout->font, renderer, val, sizeof(val),
                    layout_entry_index_pos(layout, i - first, sizeof(index)));
        }

        ui_scroll_render(&state->scroll, renderer,
            layout_entry_index_pos(layout, 0, p_obj_vm_stack_len));
    }
}

static void panel_obj_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    (void) rect;

    struct panel_obj_state *state = state_;
    panel_obj_render_id(state, renderer);
    panel_obj_render_target(state, renderer);
    panel_obj_render_cargo(state, renderer);
    panel_obj_render_docks(state, renderer);
    panel_obj_render_mod(state, renderer);
    panel_obj_render_vm(state, renderer);
}

static void panel_obj_update(struct panel_obj_state *state)
{
    assert(state->id && !coord_null(state->star));

    struct hunk *hunk = sector_hunk(core.state.sector, state->star);
    state->obj = hunk_obj(hunk, state->id);
    assert(state->obj);

    state->click.ip.disabled = !state->obj->mod;
    state->click.mod.disabled = !state->obj->mod;
    if (state->obj->mod) mods_name(state->obj->mod->id, &state->mod);

    ui_scroll_update(&state->scroll, obj_vm(state->obj)->sp);
}

static bool panel_obj_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_obj_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {

        case EV_OBJ_SELECT: {
            state->id = (uint64_t) event->user.data1;
            state->star = id_to_coord((uint64_t) event->user.data2);
            panel_obj_update(state);
            panel_show(panel);
            return true;
        }

        case EV_OBJ_CLEAR: {
            state->id = 0;
            state->star = (struct coord) {0};
            state->obj = NULL;
            panel_hide(panel);
            return true;
        }

        case EV_STATE_UPDATE: {
            if (panel->hidden) return false;
            panel_obj_update(state);
            panel_invalidate(panel);
            return false;
        }

        default: { return false; }
        }
    }

    if (panel->hidden) return false;

    {
        enum ui_ret ret = ui_click_events(&state->click.mod, event);
        if (ret & ui_action) core_push_event(EV_CODE_SELECT, state->obj->mod->id, 0);
        if (ret & ui_invalidate) panel_invalidate(panel);
        if (ret & ui_consume) return true;
    }

    {
        enum ui_ret ret = ui_click_events(&state->click.ip, event);
        if (ret & ui_action) {
            const struct vm *vm = obj_vm(state->obj);
            const struct mod *mod = state->obj->mod;
            core_push_event(EV_CODE_SELECT, mod->id, vm->ip);
        }
        if (ret & ui_invalidate) panel_invalidate(panel);
        if (ret & ui_consume) return true;
    }

    {
        enum ui_ret ret = ui_scroll_events(&state->scroll, event);
        if (ret & ui_invalidate) panel_invalidate(panel);
        if (ret & ui_consume) return true;
    }

    return false;
}


static void panel_obj_free(void *state_)
{
    struct panel_obj_state *state = state_;
    layout_free(state->layout);
    free(state);
};

struct panel *panel_obj_new(void)
{
    struct font *font_head = font_mono10;
    struct font *font = font_mono8;
    size_t menu_h = panel_menu_height();

    struct layout *layout = layout_alloc(p_obj_len,
            core.rect.w, core.rect.h - menu_h - panel_total_padding);

    layout_text(layout, p_obj_id, font_head, id_str_len, 1);
    layout_sep(layout, p_obj_id_sep);

    layout_text(layout, p_obj_target, font, sizeof(p_obj_target_str) + id_str_len, 1);
    layout_sep(layout, p_obj_target_sep);

    layout_text(layout, p_obj_cargo, font, sizeof(p_obj_cargo_str), 1);
    layout_grid(layout, p_obj_cargo_grid, p_obj_cargo_rows, p_obj_cargo_cols, p_obj_cargo_size);
    layout_sep(layout, p_obj_cargo_sep);

    layout_text(layout, p_obj_docks, font, sizeof(p_obj_docks_str), 1);
    layout_text(layout, p_obj_docks_list, font, 3 + id_str_len, p_obj_docks_max);
    layout_sep(layout, p_obj_docks_sep);

    layout_text(layout, p_obj_docks, font, sizeof(p_obj_docks_str), 1);

    layout_text(layout, p_obj_mod, font, sizeof(p_obj_mod_str) + vm_atom_cap, 1);
    layout_sep(layout, p_obj_mod_sep);

    layout_text(layout, p_obj_vm, font, sizeof(p_obj_vm_str) + p_obj_vm_specs_len, 1);
    layout_text(layout, p_obj_vm_tsc, font, sizeof(p_obj_vm_tsc_str) + p_obj_vm_u32_len, 1);
    layout_text(layout, p_obj_vm_flags, font, p_obj_vm_flags_len, 1);
    layout_text(layout, p_obj_vm_io, font, p_obj_vm_io_len, 1);
    layout_text(layout, p_obj_vm_ipsp, font, p_obj_vm_ipsp_len, 1);
    layout_sep(layout, p_obj_vm_regs_sep);
    layout_text(layout, p_obj_vm_regs, font, p_obj_vm_reg_len + p_obj_vm_u64_len, p_obj_vm_reg_rows);
    layout_sep(layout, p_obj_vm_stack_sep);
    layout_text(layout, p_obj_vm_stack, font, layout_inf, layout_inf);

    layout_finish(layout, (SDL_Point) { .x = panel_padding, .y = panel_padding });
    layout->pos = (SDL_Point) {
        .x = core.rect.w - core.ui.star->rect.w - layout->bbox.w - panel_total_padding,
        .y = menu_h
    };

    struct panel_obj_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    ui_click_init(&state->click.mod, layout_abs_rect(
                    layout, p_obj_mod, 0, sizeof(p_obj_mod_str), vm_atom_cap, 1));
    ui_click_init(&state->click.ip, layout_abs_rect(
                    layout, p_obj_vm_ipsp, 0, sizeof(p_obj_vm_ip_str), p_obj_vm_u32_len, 1));

    {
        SDL_Rect events = layout_abs(layout, p_obj_vm_stack);
        SDL_Rect bar = layout_abs_index(
                layout, p_obj_vm_stack, layout_inf, p_obj_vm_stack_len);
        ui_scroll_init(&state->scroll, &bar, &events, 0,
                layout_entry(layout, p_obj_vm_stack)->rows);
    }

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = layout->pos.x,
                .y = layout->pos.y,
                .w = layout->bbox.w + panel_total_padding,
                .h = layout->bbox.h + panel_total_padding });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_obj_render;
    panel->events = panel_obj_events;
    panel->free = panel_obj_free;
    return panel;
}
