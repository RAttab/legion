/* sound.h
   Remi Attab (remi.attab@gmail.com), 19 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// sound
// -----------------------------------------------------------------------------

enum render_sound : unsigned
{
    sound_nil = 0,

    sound_bgm_first,
    bgm_piano = sound_bgm_first,
    sound_bgm_last,

    sound_sfx_first = sound_bgm_last,
    sfx_button = sound_sfx_first,
    sound_sfx_last,

    sound_files_len = sound_sfx_last,
};

float sound_mix_master(void);
void sound_mix_master_set(float);
float sound_mix_sfx(void);
void sound_mix_sfx_set(float);
float sound_mix_bgm(void);
void sound_mix_bgm_set(float);

void sound_sfx_play(enum render_sound);
void sound_bgm_play(enum render_sound);
void sound_bgm_pause(bool);
bool sound_bgm_paused(void);
