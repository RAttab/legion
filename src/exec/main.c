/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sys.h"
#include "render/render.h"

#include <getopt.h>


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

static void usage(int code, const char *msg)
{
    if (msg) fprintf(stderr, "%s\n\n", msg);

    static const char usage[] =
        "legion [--graph] [--items] [--local] [--server|--client <host>] [--port <port>] [--file <path>]\n"
        "  -h --help    Prints this message\n"
        "  -f --file    Load and save to the given path; default is './legion.save'\n"
        "  -g --graph   Output dotfile of tape graph to stdout\n"
        "  -i --items   Dumps stats about item requirements\n"
        "  -s --local   Starts the game locally; default command if none are provided\n"
        "  -s --server  Starts a game server listening on the given host\n"
        "  -c --client  Connects to the game server at the given host\n"
        "  -p --port    Specifies the port used by the -s or -c option;\n"
        "               default port is 18181 if none are provided\n"
        "";
    fprintf(stderr, usage);
    exit(code);
}

int main(int argc, char *const argv[])
{
    const char *optstring = "+hf:gil:s:c:p:";
    struct option longopts[] = {
        { .val = 'h', .name = "help",   .has_arg = no_argument },
        { .val = 'f', .name = "file",   .has_arg = required_argument },
        { .val = 'g', .name = "graph",  .has_arg = no_argument },
        { .val = 'i', .name = "items",  .has_arg = no_argument },
        { .val = 'l', .name = "local",  .has_arg = no_argument },
        { .val = 's', .name = "server", .has_arg = required_argument },
        { .val = 'c', .name = "client", .has_arg = required_argument },
        { .val = 'p', .name = "port",   .has_arg = required_argument },
        {0},
    };

    static struct {
        bool graph, items, local, server, client;
        const char *file;
        const char *node;
        const char *service;
    } args;

    args.service = "18181"; // Dihedral Prime beause it makes me sound smart.
    args.file = "./legion.save";

    bool done = false;
    size_t commands = 0;
    while (!done) {
        switch (getopt_long(argc, argv, optstring, longopts, NULL))
        {
        case -1: { done = true; break; }
        case 'h': { usage(0, NULL); }

        case 'f': { args.file = optarg; break; }

        case 'g': { args.graph = true; commands++; break; }
        case 'i': { args.items = true; commands++; break; }
        case 'l': { args.local = true; commands++; break; }

        case 's': { args.server = true; args.node = optarg; commands++; break; }
        case 'c': { args.client = true; args.node = optarg; commands++; break; }
        case 'p': { args.service = optarg; break; }

        case '?':
        default: { usage(1, NULL); }
        }
    }

    if (optind != argc) usage(1, "unexpected arguments");

    if (commands == 0) args.local = true;
    else if (commands > 1) usage(1, "too many commands provided");

    sys_populate();

    if (args.graph && !graph_run()) return 1;
    if (args.items && !stats_run()) return 1;
    if (args.local && !local_run(args.file)) return 1;
    if (args.client && !client_run(args.node, args.service)) return 1;
    if (args.server && !server_run(args.file, args.node, args.service)) return 1;

    return 0;
}
