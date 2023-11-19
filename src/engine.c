/* engine.c
   Remi Attab (remi.attab@gmail.com), 23 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "engine.h"
#include "db.h"
#include "game.h"
#include "ux.h"

#include "utils/fs.h"
#include "utils/err.h"
#include "utils/str.h"
#include "utils/bits.h"

#include <alsa/asoundlib.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#include "engine/glad/glad.c"
#include "engine/events.c"
#include "engine/shaders.c"
#include "engine/textures.c"
#include "engine/fonts.c"
#include "engine/render.c"
#include "engine/sound.c"
#include "engine/engine.c"
