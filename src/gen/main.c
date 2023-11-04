/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/str.h"

#include <getopt.h>

// -----------------------------------------------------------------------------
// declarations
// -----------------------------------------------------------------------------

bool db_run(const char *res, const char *src);
bool tech_run(const char *res, const char *src, const char *output);


// -----------------------------------------------------------------------------
// usage
// -----------------------------------------------------------------------------

void usage(int code, const char *msg)
{
    if (msg) fprintf(stderr, "%s\n\n", msg);

    static const char usage[] =
        "gen --help\n"
        "    --db <res-path> [--src <path>]\n"
        "    --tech <tech-file> [--src <path>] [--output <path>]\n"
        "\n"
        "Commands:\n"
        "  -h --help    Prints this message\n"
        "  -D --db      Generate the database from the item files\n"
        "  -E --tech    Generate the tech tree from the tech files\n"
        "\n"
        "Arguments:\n"
        "  -s --src     Where to write source files\n"
        "  -o --output  Where auxiliary files are written\n"
        "";
    fprintf(stderr, usage);
    exit(code);
}


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int argc, char *const argv[])
{
    const char *optstring = "+hD:T:s:o:";
    struct option longopts[] = {
        { .val = 'h', .name = "help",   .has_arg = no_argument },

        { .val = 'D', .name = "db",     .has_arg = required_argument },
        { .val = 'T', .name = "tech",   .has_arg = required_argument },

        { .val = 's', .name = "src",    .has_arg = required_argument },
        { .val = 'o', .name = "output", .has_arg = required_argument },

        {0},
    };

    enum { cmd_nil = 0, cmd_db, cmd_tech } cmd = cmd_nil;

    static struct {
        const char *db;
        const char *tech;
        const char *src;
        const char *output;
    } args = { .src = "./src/db/gen", .output = "." };

    bool done = false;
    size_t commands = 0;
    while (!done) {
        switch (getopt_long(argc, argv, optstring, longopts, NULL))
        {
        case -1: { done = true; break; }

        case 'h': { usage(0, NULL); }
        case 'D': { cmd = cmd_db; commands++; args.db = optarg; break; }
        case 'T': { cmd = cmd_tech; commands++; args.tech = optarg; break; }

        case 'o': { args.output = optarg; break; }
        case 's': { args.output = optarg; break; }

        case '?':
        default: { usage(1, NULL); }
        }
    }

    if (optind != argc) usage(1, "unexpected arguments");
    if (commands < 1) usage(1, "no commands provided");
    if (commands > 1) usage(1, "too many commands provided");

    // We don't want to run engine_populate for this command.
    if (cmd == cmd_db)
        return db_run(args.db, args.src) ? 0 : 1;
    
    if (cmd == cmd_tech)
        return tech_run(args.tech, args.src, args.output) ? 0 : 1;

    assert(false);
}
