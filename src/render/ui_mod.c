/* ui_mod.c
   Rémi Attab (remi.attab@gmail.com), 18 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "vm/mod.h"


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

struct ui_mod
{
    struct ui_panel panel;
    struct ui_code code;

    struct mod *mod;
};

static struct font *ui_mod_font(void) { return font_mono8; }

struct ui_mod *ui_mod_new(void)
{
    struct pos pos = make_pos(
            ui_mods_width(core.ui.mods),
            ui_topbar_height(core.ui.topbar));

    struct dim dim = make_dim(
            (ui_code_num_len+1 + text_line_cap + 2) * ui_mod_font()->glyph_w,
            core.rect.h - pos.y);

    struct ui_mod *mod = calloc(1, sizeof(*mod));
    *mod = (struct ui_mod) {
        .panel = ui_panel_title(pos, dim, ui_str_v(vm_atom_cap)),
        .code = ui_code_new(make_dim(ui_layout_inf, ui_layout_inf), ui_mod_font())
    };
    mod->panel.state = ui_panel_hidden;
    return mod;

}

void ui_mod_free(struct ui_mod *mod)
{
    ui_panel_free(&mod->panel);
    ui_code_free(&mod->code);
    mod_discard(mod->mod);
    free(mod);
}

static bool ui_mod_event_user(struct ui_mod *mod, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_MOD_SELECT: {
        mod_t id = (uintptr_t) ev->user.data1;
        ip_t ip = (uintptr_t) ev->user.data2;

        mod->mod = mods_load(id);
        assert(mod->mod);
        ui_code_set(&mod->code, mod->mod, ip);

        atom_t name = {0};
        mods_name(id, &name);
        ui_str_setf(&mod->panel.title.str, "mod: %s", name);

        mod->panel.state = ui_panel_visible;
        core_push_event(EV_FOCUS, (uintptr_t) &mod->panel, 0);

        return false;
    }

    case EV_MOD_CLEAR: {
        ui_code_clear(&mod->code);
        mod_discard(mod->mod);
        mod->mod = NULL;
        mod->panel.state = ui_panel_hidden;
        return false;
    }

    case EV_STATE_UPDATE: {
        ui_code_tick(&mod->code, core.ticks);
        return false;
    }

    default: { return false; }
    }
}

bool ui_mod_event(struct ui_mod *mod, SDL_Event *ev)
{
    if (ev->type == core.event && ui_mod_event_user(mod, ev)) return true;

    enum ui_ret ret = ui_nil;

    if ((ret = ui_panel_event(&mod->panel, ev))) {
        if (ret == ui_consume && mod->panel.state == ui_panel_hidden)
            core_push_event(EV_MOD_CLEAR, 0, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_code_event(&mod->code, ev))) return ret == ui_consume;

    return false;
}

void ui_mod_render(struct ui_mod *mod, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&mod->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_code_render(&mod->code, &layout, renderer);
}
