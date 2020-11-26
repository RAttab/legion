#! /usr/bin/env bash

set -o errexit -o nounset -o pipefail # -o xtrace

: ${PREFIX:="."}

declare -a SRC
SRC=(rng coord htable text sector vm color font panel map core game)

declare -a TEST
TEST=(coord)

CC=${CC:-gcc}

CFLAGS="-ggdb -O3 -march=native -pipe -std=gnu11 -D_GNU_SOURCE"
CFLAGS="$CFLAGS -I${PREFIX}/src"
CFLAGS="$CFLAGS $(sdl2-config --cflags)"

CFLAGS="$CFLAGS -Wall -Wextra"
CFLAGS="$CFLAGS -Wundef"
CFLAGS="$CFLAGS -Wcast-align"
CFLAGS="$CFLAGS -Wwrite-strings"
CFLAGS="$CFLAGS -Wunreachable-code"
CFLAGS="$CFLAGS -Wformat=2"
CFLAGS="$CFLAGS -Wswitch-enum"
CFLAGS="$CFLAGS -Winit-self"
CFLAGS="$CFLAGS -Wno-strict-aliasing"
CFLAGS="$CFLAGS -fno-strict-aliasing"
CFLAGS="$CFLAGS -Wno-implicit-fallthrough"

CFLAGS="$CFLAGS $(pkg-config --cflags freetype2)"

LIBS="liblegion.a"
LIBS="$LIBS $(sdl2-config --libs)"
LIBS="$LIBS $(pkg-config --libs freetype2)"

OBJ=""
for src in "${SRC[@]}"; do
    $CC -c -o "$src.o" "${PREFIX}/src/$src.c" $CFLAGS
    OBJ="$OBJ $src.o"
done
ar rcs liblegion.a $OBJ

$CC -o "legion" "${PREFIX}/src/main.c" $LIBS $CFLAGS

cp -r "${PREFIX}/res" .

for test in "${TEST[@]}"; do
    echo $test
    $CC -o "test_$test" "${PREFIX}/test/${test}_test.c" $LIBS $CFLAGS
    "./test_$test"
done

# for test in "${TEST[@]}"; do
#     valgrind \
#         --leak-check=full \
#         --track-origins=yes \
#         --trace-children=yes \
#         --error-exitcode=1 \
#         "./test_$test"
# done
