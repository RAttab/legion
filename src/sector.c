/* sector.c
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "sector.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


// -----------------------------------------------------------------------------
// consts
// -----------------------------------------------------------------------------

static uint64_t stars_max = 1UL << 10;


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------


static size_t sector_size(size_t stars)
{
    return sizeof(struct sector) + sizeof(struct star) * stars;
}

static size_t sector_size_max()
{
    return sector_size(stars_max);
}

struct sector *sector_gen(struct coord coord)
{
    size_t size = sector_size_max();

    struct sector *sector =
        mmap(0, size, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (sector == MAP_FAILED) {
        legion_fail_errno("unable to mmap sector %llx", coord_to_id(coord));
        return NULL;
    }

    sector.len = size;
    sector.coord = coord;
    gen_sector(sector);

    return sector;
}

struct sector *sector_save(struct sector *sector, const char *path)
{
    size_t size = sector_size(sector->stars_len);

    int fd = open(path, O_RDWR | O_EXCL | O_CREAT, 0660);
    if (fd < 0) {
        legion_fail_errno("unable to create sector %llx at '%s'", coord_to_id(coord), path);
        goto fail_open;
    }


    struct sector *saved = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (sector == MAP_FAILED) {
        legion_fail_errno("unable to mmap sector %llx from '%s'", coord_to_id(coord), path);
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
        legion_fail_errno("unable to open sector at '%s'", path);
        goto fail_open;
    }
    
    struct stat stat;
    if (fstat(fd, &stat) < 0) {
        goto fail_stat;
    }
    size_t size = stat.st_size;

    
    struct sector *sector = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (sector == MAP_FAILED) {
        legion_fail_errno("unable to mmap sector from '%s'", path);
        goto fail_mmap;
    }

    munmap(saved, size);
  fail_mmap:

  fail_stat:
    
    close(fd);
  fail_open:

    return NULL;
}

void sector_close(struct sector *sector)
{
    if (!sector) return;
    
    if (mummap(sector, sector->len) < 0) {
        fail_errno("unable to munmap %llx", coord_to_id(sector->coord));
        fail_abort();
    }
}


// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

void gen_sector(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_id(sector);
    struct rng rng = rng_make(id);

    size_t stars = rng_gen_range(rng, 0, rng_gen_range(rng, 0, stars_max));
    for (size_t i = 0; i < stars; ++i) {

        struct coord coord = (struct coord) {
            .x = coord.x + rng_gen_range(rng, 0, coord_sector_max),
            .y = coord.y + rng_gen_range(rng, 0, coord_sector_max),
        };
        struct star star = star_gen(coord);
    }
}

struct star gen_star(struct coord coord)
{
    struct star star = {0};

    uint64_t id = coord_to_id(coord);
    struct rng rng = rng_make(id);

    size_t planets = rng_gen_range(rng, 1, 16);
    for (size_t planet = 0; planet < planets; ++planet) {

        size_t size = rng_gen_range(rng, 1, rng_gen_range(rng, 1, 16));
        size_t diversity = rng_gen_range(rng, 1, rng_gen_range(rng, 1, 16));

        for (size_t roll = 0; roll < diversity; ++roll) {
            size_t element = rng_gen_range(rng, 0, rng_gen_range(rng, 0, 16));
            uint16_t quantity = 1 << (size / 2 + element / 4);

            star.elements[element] += quantity;
            if (star.elements[element] < quantity) {
                star.elements[element] = UINT16_MAX;
            }
        }
    }

    return star;
}
