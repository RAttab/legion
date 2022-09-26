/* ui_man.c
   Rémi Attab (remi.attab@gmail.com), 18 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "game/man.h"


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

enum { ui_man_cols = 90, ui_man_depth = 4 };

struct ui_man
{
    struct ui_panel panel;
    struct ui_tree toc;
    struct ui_doc doc;

    struct link page;
    struct tape_set known;
};


static void ui_man_toc(
        struct ui_man *ui, const struct toc *toc, ui_node parent)
{
    for (size_t i = 0; i < toc->len; ++i) {
        const struct toc *child = toc->nodes + i;

        if (child->item && !tech_known(proxy_tech(render.proxy), child->item))
            continue;

        uint64_t user = link_to_u64(child->link);
        if (!user) failf("missing man page for '%s'", child->name);

        ui_node node = ui_tree_index(&ui->toc);
        ui_str_setv(ui_tree_add(&ui->toc, parent, user), child->name, sizeof(child->name));
        ui_man_toc(ui, child, node);
    }
}

struct ui_man *ui_man_new(void)
{
    struct font *font = make_font(font_small, font_nil);

    size_t tree_width = (man_toc_max + 2*ui_man_depth + 1) * font->glyph_w;
    size_t doc_width = (ui_man_cols + 1) * font->glyph_w;
    size_t width = tree_width + doc_width + font->glyph_w;
    struct pos pos = make_pos(render.rect.w - width, ui_topbar_height());
    struct dim dim = make_dim(width, render.rect.h - pos.y - ui_status_height());

    struct ui_man *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_man) {
        .panel = ui_panel_title(pos, dim, ui_str_c("man")),
        .toc = ui_tree_new(make_dim(tree_width, ui_layout_inf), font, man_toc_max),
        .doc = ui_doc_new(make_dim(doc_width, ui_layout_inf), font_small),
        .page = link_home(),
    };

    ui_man_toc(ui, man_toc(), ui_node_nil);
    ui_doc_open(&ui->doc, link_home(), proxy_lisp(render.proxy));

    return ui;
}

void ui_man_free(struct ui_man *ui)
{
    ui_panel_free(&ui->panel);
    ui_tree_free(&ui->toc);
    ui_doc_free(&ui->doc);
}

static void ui_man_update(struct ui_man *ui)
{
    const struct tech *tech = proxy_tech(render.proxy);
    if (tape_set_eq(&tech->known, &ui->known)) return;
    ui->known = tech->known;

    ui_tree_reset(&ui->toc);
    ui_man_toc(ui, man_toc(), ui_node_nil);
}


static bool ui_man_event_user(struct ui_man *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (ui_panel_is_visible(&ui->panel))
            ui_man_update(ui);
        return false;
    }

    case EV_MAN_TOGGLE: {
        if (ui_panel_is_visible(&ui->panel))
            ui_panel_hide(&ui->panel);
        else ui_panel_show(&ui->panel);
        return false;
    }

    case EV_MAN_GOTO: {
        uint64_t user = (uintptr_t) ev->user.data1;
        struct link link = link_from_u64(user);
        enum item item = man_item(link.page);

        if (!item || tech_known(proxy_tech(render.proxy), item))
            ui_tree_select(&ui->toc, user);
        else {
            ui_tree_clear(&ui->toc);
            link = man_sys_locked();
        }

        ui_doc_open(&ui->doc, link, proxy_lisp(render.proxy));
        return false;
    }

    case EV_STAR_SELECT: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    default: { return false; }
    }
}

bool ui_man_event(struct ui_man *ui, SDL_Event *ev)
{
    if (ev->type == render.event) return ui_man_event_user(ui, ev);

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret != ui_skip;

    if ((ret = ui_doc_event(&ui->doc, ev))) return true;

    if ((ret = ui_tree_event(&ui->toc, ev))) {
        if (ret == ui_action) {
            struct link link = link_from_u64(ui->toc.selected);
            ui_doc_open(&ui->doc, link, proxy_lisp(render.proxy));
        }
        return true;
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_man_render(struct ui_man *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_tree_render(&ui->toc, &layout, renderer);
    ui_doc_render(&ui->doc, &layout, renderer);
}
