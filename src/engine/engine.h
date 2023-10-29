/* engine.h
   Remi Attab (remi.attab@gmail.com), 23 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/types.h"
#include "game/id.h"
#include "game/coord.h"
#include "utils/err.h"
#include "utils/color.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "engine/types.h"

// -----------------------------------------------------------------------------
// engine
// -----------------------------------------------------------------------------

void engine_init(void);
void engine_close(void);
bool engine_initialized(void);

struct rect engine_area(void);
struct dim engine_viewport(void);
GLFWwindow *engine_window(void);

struct dpi { float h, v; };
struct dpi engine_dpi(void);

struct dim engine_cell(void);
unit engine_cell_baseline(void);
struct dim engine_dim(unit rows, unit cols);
struct dim engine_dim_margin(unit rows, unit cols, struct dim margin);

void engine_loop(void);
bool engine_done(void);
void engine_fork(void);
void engine_join(void);
void engine_quit(void);

// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "engine/render.h"
#include "engine/events.h"