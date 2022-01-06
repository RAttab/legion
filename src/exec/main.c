/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sys.h"
#include "game/user.h"
#include "render/render.h"

#include <getopt.h>

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

static void usage(int code, const char *msg)
{
    if (msg) fprintf(stderr, "%s\n\n", msg);

    static const char usage[] =
        "legion --help\n"
        "       --graph\n"
        "       --items\n"
        "       --token\n"
        "       --local [--file <path>] [--seed <seed>]\n"
        "       --client <host> [--port <port>] [--config <path>]\n"
        "       --server <host> [--port <port>] [--file <path>] [--config <path>]\n"
        "                       [--seed <seed>]\n"
        "\n"
        "Commands:\n"
        "  -h --help    Prints this message\n"
        "  -G --graph   Output dotfile of tape graph to stdout\n"
        "  -I --items   Dumps stats about item requirements\n"
        "  -T --token   Generates an auth token\n"
        "  -L --local   Starts the game locally; default command\n"
        "  -S --server  Starts a game server listening on the given host\n"
        "  -C --client  Connects to the game server at the given host\n"
        "\n"
        "Arguments:\n"
        "  -f --file    Path to save file; default is './legion.save'\n"
        "  -c --config  Path to config file; default is './legion.lisp'\n"
        "  -p --port    Specifies the port used by the -s or -c option;\n"
        "               default port is 18181 if none are provided\n"
        "  -s --seed    See when creating a new world formated as n 64 bit\n"
        "               hexadecimal value; Defaults to 0\n"
        "";
    fprintf(stderr, usage);
    exit(code);
}

int main(int argc, char *const argv[])
{
    const char *optstring = "+hGITLS:C:f:c:p:s:";
    struct option longopts[] = {
        { .val = 'h', .name = "help",   .has_arg = no_argument },

        { .val = 'G', .name = "graph",  .has_arg = no_argument },
        { .val = 'I', .name = "items",  .has_arg = no_argument },
        { .val = 'T', .name = "token",  .has_arg = no_argument },
        { .val = 'L', .name = "local",  .has_arg = no_argument },
        { .val = 'S', .name = "server", .has_arg = required_argument },
        { .val = 'C', .name = "client", .has_arg = required_argument },

        { .val = 'f', .name = "file",   .has_arg = required_argument },
        { .val = 'c', .name = "config", .has_arg = required_argument },
        { .val = 'p', .name = "port",   .has_arg = required_argument },
        { .val = 's', .name = "seed",   .has_arg = required_argument },
        {0},
    };

    enum {
        cmd_nil = 0,
        cmd_graph, cmd_items, cmd_token,
        cmd_local, cmd_server, cmd_client,
    } cmd = cmd_nil;

    static struct {
        const char *save;
        const char *config;
        const char *node;
        const char *service;
        seed_t seed;
    } args = {
        .save = "./legion.save",
        .config = "./legion.lisp",
        .service = "18181", // Dihedral Prime beause it makes me sound smart.
        .seed = 0,
    };

    bool done = false;
    size_t commands = 0;
    while (!done) {
        switch (getopt_long(argc, argv, optstring, longopts, NULL))
        {
        case -1: { done = true; break; }

        case 'h': { usage(0, NULL); }
        case 'G': { cmd = cmd_graph; commands++; break; }
        case 'I': { cmd = cmd_items; commands++; break; }
        case 'L': { cmd = cmd_local; commands++; break; }
        case 'S': { cmd = cmd_server; commands++; args.node = optarg; break; }
        case 'C': { cmd = cmd_client; commands++; args.node = optarg; break; }

        case 'f': { args.save = optarg; break; }
        case 'c': { args.config = optarg; break; }
        case 'p': { args.service = optarg; break; }
        case 's': {
            size_t len = strlen(optarg);
            if (str_atox(optarg, len, args.seed) != len)
                usage(1, "invalid seed argument");
            break;
        };

        case '?':
        default: { usage(1, NULL); }
        }
    }

    if (optind != argc) usage(1, "unexpected arguments");

    if (commands == 0) cmd = cmd_local;
    else if (commands > 1) usage(1, "too many commands provided");

    sys_populate();

    bool ok = false;
    switch (cmd)
    {
    case cmd_graph:  { ok = graph_run(); break; }
    case cmd_items:  { ok = stats_run(); break; }
    case cmd_local:  { ok = local_run(args.file, args.seed); break; }
    case cmd_token:  { ok = true; fprintf(stdout, "%lx\n", token()); break; }
    case cmd_client: { ok = client_run(args.node, args.service); break; }
    case cmd_server: {
        ok = server_run(args.node, args.service, args.save, args.config, args.seed);
        break;
    }
    default: { assert(false); }
    }

    return ok ? 0 : 1;
}
