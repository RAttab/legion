/* args.h
   Rémi Attab (remi.attab@gmail.com), 07 Jan 2024
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// args
// -----------------------------------------------------------------------------

struct args
{
    const char *save;
    const char *config;
    const char *db;
    const char *tech;
    const char *node;
    const char *service;
    const char *type;
    const char *metrics;
    world_seed seed;
    user_token auth;
    struct symbol name;
};
