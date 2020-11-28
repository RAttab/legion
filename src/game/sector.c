/* sector.c
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "sector.h"

#include "rng.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


// -----------------------------------------------------------------------------
// consts
// -----------------------------------------------------------------------------

static uint64_t stars_max = 1UL << 10;



// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------


struct system_desc *system_gen(struct coord coord)
{
    uint64_t id = coord_to_id(coord);
    struct rng rng = rng_make(id);
    size_t planets = rng_uni(&rng, 1, 16);

    struct system_desc *system =
        calloc(1, sizeof(*system) + planets * sizeof(system->planets[0]));
    *system = (struct system_desc) {
        .s = (struct system) { .coord = coord },
        .planets_len = planets
    };

    system->s.star = 1 << rng_uni(&rng, 1, 31);

    for (size_t planet = 0; planet < planets; ++planet) {
        size_t size = rng_exp(&rng, 1, 16);
        size_t diversity = rng_exp(&rng, 1, 16);

        for (size_t roll = 0; roll < diversity; ++roll) {
            size_t element = rng_exp(&rng, 1, 16);
            uint16_t quantity = 1 << (size / 2 + element / 4);
            
            system->s.elements[element] += quantity;
            system->planets[planet][element] += quantity;
        }
    }

    return system;
}

void gen_sector(struct sector *sector)
{
    uint64_t id = coord_to_id(sector->coord);
    struct rng rng = rng_make(id);

    sector->systems_len = rng_exp(&rng, 0, stars_max);
    for (size_t i = 0; i < sector->systems_len; ++i) {

        struct coord coord = (struct coord) {
            .x = coord.x + rng_uni(&rng, 0, coord_system_max),
            .y = coord.y + rng_uni(&rng, 0, coord_system_max),
        };

        struct system_desc *system = system_gen(coord);
        sector->systems[i] = system->s;
    }
}


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

static size_t sector_size(size_t stars)
{
    return sizeof(struct sector) + sizeof(struct system) * stars;
}

static size_t sector_size_max()
{
    return sector_size(stars_max);
}

struct sector *sector_gen(struct coord coord)
{
    size_t size = sector_size_max();

    struct sector *sector =
        mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (sector == MAP_FAILED) {
        SDL_LogErrno("unable to mmap sector %lx", coord_to_id(coord));
        return NULL;
    }

    *sector = (struct sector) {
        .len = size,
        .coord = coord,
    };
    gen_sector(sector);

    if (mprotect(sector, size, PROT_READ) < 0) {
        SDL_LogErrno("unable to mprotect read %lx", coord_to_id(coord));
    }

    return sector;
}

struct sector *sector_save(struct sector *sector, const char *path)
{
    size_t size = sector_size(sector->systems_len);

    int fd = open(path, O_RDWR | O_EXCL | O_CREAT, 0660);
    if (fd < 0) {
        SDL_LogErrno("unable to create sector %lx at '%s'",
                coord_to_id(sector->coord), path);
        goto fail_open;
    }


    struct sector *saved = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (sector == MAP_FAILED) {
        SDL_LogErrno("unable to mmap sector %lx from '%s'",
                coord_to_id(sector->coord), path);
        goto fail_mmap;
    }

    memcpy(saved, sector, size);
    saved->len = size;

    (void) close(fd);
    sector_close(sector);

    return saved;

    munmap(saved, size);
  fail_mmap:

    close(fd);
  fail_open:

    return NULL;
}

struct sector *sector_load(const char *path)
{
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        SDL_LogErrno("unable to open sector at '%s'", path);
        goto fail_open;
    }

    struct stat stat;
    if (fstat(fd, &stat) < 0) {
        goto fail_stat;
    }
    size_t size = stat.st_size;


    struct sector *sector = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (sector == MAP_FAILED) {
        SDL_LogErrno("unable to mmap sector from '%s'", path);
        goto fail_mmap;
    }

    return sector;

    munmap(sector, size);
  fail_mmap:

  fail_stat:

    close(fd);
  fail_open:

    return NULL;
}

void sector_close(struct sector *sector)
{
    if (!sector) return;

    if (munmap(sector, sector->len) < 0) {
        SDL_LogErrno("unable to munmap %lx", coord_to_id(sector->coord));
        abort();
    }
}


struct system *sector_lookup(struct sector *sector, struct rect *rect)
{
    for (size_t i = 0; i < sector->systems_len; ++i) {
        struct system *system = &sector->systems[i];
        if (rect_contains(rect, system->coord)) {
            return system;
        }
    }
    return NULL;
}
