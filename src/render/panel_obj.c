/* pobj.c
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

struct pobj_state
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
    pobj_id = 0,
    pobj_id_sep,
    pobj_target,
    pobj_target_sep,
    pobj_cargo,
    pobj_cargo_grid,
    pobj_cargo_sep,
    pobj_docks,
    pobj_docks_list,
    pobj_docks_sep,
    pobj_mod,
    pobj_mod_sep,
    pobj_vm,
    pobj_vm_tsc,
    pobj_vm_flags,
    pobj_vm_io,
    pobj_vm_ipsp,
    pobj_vm_regs_sep,
    pobj_vm_regs,
    pobj_vm_stack_sep,
    pobj_vm_stack,
    pobj_len,
};

static const char pobj_target_str[] = "target:";
static const char pobj_cargo_str[] = "cargo:";
static const char pobj_docks_str[] = "docks:";
static const char pobj_mod_str[] = "mod:";

static const char pobj_vm_str[] = "vm:";
static const char pobj_vm_tsc_str[] = "tsc:";
static const char pobj_vm_flags_str[] = "flags:";
static const char pobj_vm_io_str[] = "io:";
static const char pobj_vm_ior_str[] = "ior:";
static const char pobj_vm_ip_str[] = "ip:";
static const char pobj_vm_sp_str[] = "sp:";

enum
{
    pobj_cargo_cols = 5,
    pobj_cargo_rows = 2,
    pobj_cargo_size = 25,
    pobj_docks_max = 5,

    pobj_vm_u64_len = 16,
    pobj_vm_u32_len = 8,
    pobj_vm_u8_len = 2,

    pobj_vm_flag_len = 3,
    pobj_vm_flags_len = (
            sizeof(pobj_vm_flags_str) + pobj_vm_flag_len * 8), // 'IO SU FR FS ...'

    pobj_vm_spec_len = 3,
    pobj_vm_specs_len = pobj_vm_spec_len * 2 + 1, // '01/02'

    pobj_vm_io_len = (
            sizeof(pobj_vm_io_str) + pobj_vm_u8_len + 1 +
            sizeof(pobj_vm_ior_str) + pobj_vm_u8_len),

    pobj_vm_ipsp_len = (
            sizeof(pobj_vm_ip_str) + pobj_vm_u32_len + 1 +
            sizeof(pobj_vm_sp_str) + pobj_vm_u8_len),

    pobj_vm_reg_len = 4, // 'rX: '
    pobj_vm_reg_rows = 4,

    pobj_vm_stack_prefix_len = 5, // 'sXX: '
    pobj_vm_stack_len = pobj_vm_stack_prefix_len + pobj_vm_u64_len,
    pobj_vm_stack_total_len = pobj_vm_stack_len + ui_scroll_layout_cols,
};


static void pobj_render_id(struct pobj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, pobj_id);
    char str[id_str_len];
    id_str(state->id, sizeof(str), str);
    font_render(layout->font, renderer, str, sizeof(str), layout_entry_pos(layout));
}

static void pobj_render_target(struct pobj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, pobj_target);
    font_render(
            layout->font, renderer,
            pobj_target_str, sizeof(pobj_target_str),
            layout_entry_pos(layout));

    if (state->obj->target) {
        char str[id_str_len];
        id_str(state->obj->target, sizeof(str), str);
        font_render(layout->font, renderer, str, sizeof(str),
                layout_entry_index_pos(layout, 0, sizeof(pobj_target)));
    }
}

static void pobj_render_cargo(struct pobj_state *state, SDL_Renderer *renderer)
{
    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_cargo);
        font_render(
                layout->font, renderer,
                pobj_cargo_str, sizeof(pobj_cargo_str),
                layout_entry_pos(layout));
    }

    struct layout_entry *layout = layout_entry(state->layout, pobj_cargo_grid);

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

static void pobj_render_docks(struct pobj_state *state, SDL_Renderer *renderer)
{
    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_docks);
        font_render(
                layout->font, renderer,
                pobj_docks_str, sizeof(pobj_docks_str),
                layout_entry_pos(layout));
    }

    struct layout_entry *layout = layout_entry(state->layout, pobj_docks_list);

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

static void pobj_render_mod(struct pobj_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, pobj_mod);
    font_render(layout->font, renderer, pobj_mod_str, sizeof(pobj_mod_str),
            layout_entry_pos(layout));

    if (state->obj->mod) {
        SDL_Point pos = layout_entry_index_pos(layout, 0, sizeof(pobj_mod_str));
        ui_click_render(&state->click.mod, renderer, pos);
        font_render(layout->font, renderer, state->mod, sizeof(state->mod), pos);
    }
}

static void pobj_render_vm(struct pobj_state *state, SDL_Renderer *renderer)
{
    const struct vm *vm = obj_vm(state->obj);

    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_vm);
        SDL_Point pos = layout_entry_pos(layout);

        font_render(layout->font, renderer, pobj_vm_str, sizeof(pobj_vm_str), pos);

        const size_t len = pobj_vm_spec_len;

        char val[pobj_vm_specs_len];
        str_utoa(vm->specs.stack, val, len);
        val[len] = '/';
        str_utoa(vm->specs.speed, val + len + 1, len);

        pos.x = state->layout->bbox.w - (layout->item.w * sizeof(val));
        font_render(layout->font, renderer, val, sizeof(val), pos);
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_vm_tsc);
        font_render(layout->font, renderer, pobj_vm_tsc_str, sizeof(pobj_vm_tsc_str),
                layout_entry_pos(layout));

        char val[pobj_vm_u32_len];
        str_utox(vm->tsc, val, sizeof(val));
        font_render(layout->font, renderer, val, sizeof(val),
                layout_entry_index_pos(layout, 0, sizeof(pobj_vm_tsc_str)));
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_vm_flags);
        size_t col = 0;

        font_render(layout->font, renderer, pobj_vm_flags_str, sizeof(pobj_vm_flags_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(pobj_vm_flags_str);

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
        struct layout_entry *layout = layout_entry(state->layout, pobj_vm_io);
        size_t col = 0;

        font_render(layout->font, renderer, pobj_vm_io_str, sizeof(pobj_vm_io_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(pobj_vm_io_str);

        char io[pobj_vm_u8_len];
        str_utox(vm->io, io, sizeof(io));
        font_render(layout->font, renderer, io, sizeof(io),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(io) + 1;

        font_render(layout->font, renderer, pobj_vm_ior_str, sizeof(pobj_vm_ior_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(pobj_vm_ior_str);

        char ior[pobj_vm_u8_len];
        str_utox(vm->ior, ior, sizeof(ior));
        font_render(layout->font, renderer, ior, sizeof(ior),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(ior) + 1;
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_vm_ipsp);
        size_t col = 0;

        font_render(layout->font, renderer, pobj_vm_ip_str, sizeof(pobj_vm_ip_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(pobj_vm_ip_str);

        char ip[pobj_vm_u32_len];
        str_utox(vm->ip, ip, sizeof(ip));
        ui_click_render(&state->click.ip, renderer,
                layout_entry_index_pos(layout, 0, col));
        font_render(layout->font, renderer, ip, sizeof(ip),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(ip) + 1;

        font_render(layout->font, renderer, pobj_vm_sp_str, sizeof(pobj_vm_sp_str),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(pobj_vm_sp_str);

        char sp[pobj_vm_u8_len];
        str_utox(vm->sp, sp, sizeof(sp));
        font_render(layout->font, renderer, sp, sizeof(sp),
                layout_entry_index_pos(layout, 0, col));
        col += sizeof(sp) + 1;
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_vm_regs);

        for (size_t i = 0; i < pobj_vm_reg_rows; ++i) {
            char reg[pobj_vm_reg_len] = { '$', '1'+i, ':', 0};
            font_render(layout->font, renderer, reg, sizeof(reg),
                    layout_entry_index_pos(layout, i, 0));

            char val[pobj_vm_u64_len];
            str_utox(vm->regs[i], val, sizeof(val));
            font_render(layout->font, renderer, val, sizeof(val),
                    layout_entry_index_pos(layout, i, sizeof(reg)));
        }
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, pobj_vm_stack);

        const size_t first = state->scroll.first;
        const size_t rows = u64_min(vm->sp, state->scroll.visible);

        for (size_t i = first; i < first + rows; ++i) {
            size_t sp = vm->sp - i - 1;

            char index[pobj_vm_stack_prefix_len] = {
                's', str_hexchar(sp >> 4), str_hexchar(sp), ':', 0 };
            font_render(layout->font, renderer, index, sizeof(index),
                    layout_entry_index_pos(layout, i - first, 0));

            char val[pobj_vm_u64_len];
            str_utox(vm->stack[sp], val, sizeof(val));
            font_render(layout->font, renderer, val, sizeof(val),
                    layout_entry_index_pos(layout, i - first, sizeof(index)));
        }

        ui_scroll_render(&state->scroll, renderer,
            layout_entry_index_pos(layout, 0, pobj_vm_stack_len));
    }
}

static void pobj_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    (void) rect;

    struct pobj_state *state = state_;
    pobj_render_id(state, renderer);
    pobj_render_target(state, renderer);
    pobj_render_cargo(state, renderer);
    pobj_render_docks(state, renderer);
    pobj_render_mod(state, renderer);
    pobj_render_vm(state, renderer);
}

static void pobj_update(struct pobj_state *state)
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

static bool pobj_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct pobj_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {

        case EV_OBJ_SELECT: {
            state->id = (uint64_t) event->user.data1;
            state->star = id_to_coord((uint64_t) event->user.data2);
            pobj_update(state);
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
            pobj_update(state);
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


static void pobj_free(void *state_)
{
    struct pobj_state *state = state_;
    layout_free(state->layout);
    free(state);
};

struct panel *panel_obj_new(void)
{
    struct font *font_head = font_mono10;
    struct font *font = font_mono8;
    size_t menu_h = panel_menu_height();

    struct layout *layout = layout_alloc(pobj_len,
            core.rect.w, core.rect.h - menu_h - panel_total_padding);

    layout_text(layout, pobj_id, font_head, id_str_len, 1);
    layout_sep(layout, pobj_id_sep);

    layout_text(layout, pobj_target, font, sizeof(pobj_target_str) + id_str_len, 1);
    layout_sep(layout, pobj_target_sep);

    layout_text(layout, pobj_cargo, font, sizeof(pobj_cargo_str), 1);
    layout_grid(layout, pobj_cargo_grid, pobj_cargo_rows, pobj_cargo_cols, pobj_cargo_size);
    layout_sep(layout, pobj_cargo_sep);

    layout_text(layout, pobj_docks, font, sizeof(pobj_docks_str), 1);
    layout_text(layout, pobj_docks_list, font, 3 + id_str_len, pobj_docks_max);
    layout_sep(layout, pobj_docks_sep);

    layout_text(layout, pobj_docks, font, sizeof(pobj_docks_str), 1);

    layout_text(layout, pobj_mod, font, sizeof(pobj_mod_str) + vm_atom_cap, 1);
    layout_sep(layout, pobj_mod_sep);

    layout_text(layout, pobj_vm, font, sizeof(pobj_vm_str) + pobj_vm_specs_len, 1);
    layout_text(layout, pobj_vm_tsc, font, sizeof(pobj_vm_tsc_str) + pobj_vm_u32_len, 1);
    layout_text(layout, pobj_vm_flags, font, pobj_vm_flags_len, 1);
    layout_text(layout, pobj_vm_io, font, pobj_vm_io_len, 1);
    layout_text(layout, pobj_vm_ipsp, font, pobj_vm_ipsp_len, 1);
    layout_sep(layout, pobj_vm_regs_sep);
    layout_text(layout, pobj_vm_regs, font, pobj_vm_reg_len + pobj_vm_u64_len, pobj_vm_reg_rows);
    layout_sep(layout, pobj_vm_stack_sep);
    layout_text(layout, pobj_vm_stack, font, layout_inf, layout_inf);

    layout_finish(layout, (SDL_Point) { .x = panel_padding, .y = panel_padding });
    layout->pos = (SDL_Point) {
        .x = core.rect.w - core.ui.star->rect.w - layout->bbox.w - panel_total_padding,
        .y = menu_h
    };

    struct pobj_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    ui_click_init(&state->click.mod, layout_abs_rect(
                    layout, pobj_mod, 0, sizeof(pobj_mod_str), vm_atom_cap, 1));
    ui_click_init(&state->click.ip, layout_abs_rect(
                    layout, pobj_vm_ipsp, 0, sizeof(pobj_vm_ip_str), pobj_vm_u32_len, 1));

    {
        SDL_Rect events = layout_abs(layout, pobj_vm_stack);
        SDL_Rect bar = layout_abs_index(
                layout, pobj_vm_stack, layout_inf, pobj_vm_stack_len);
        ui_scroll_init(&state->scroll, &bar, &events, 0,
                layout_entry(layout, pobj_vm_stack)->rows);
    }

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = layout->pos.x,
                .y = layout->pos.y,
                .w = layout->bbox.w + panel_total_padding,
                .h = layout->bbox.h + panel_total_padding });
    panel->hidden = true;
    panel->state = state;
    panel->render = pobj_render;
    panel->events = pobj_events;
    panel->free = pobj_free;
    return panel;
}
