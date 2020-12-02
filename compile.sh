#! /usr/bin/env bash

set -o errexit -o nounset -o pipefail # -o xtrace

: ${PREFIX:="."}

declare -a SRC
SRC=(utils vm game render)

declare -a TEST
TEST=(coord vm)

CC=${CC:-gcc}

CFLAGS="-ggdb -O3 -march=native -pipe -std=gnu11 -D_GNU_SOURCE"
CFLAGS="$CFLAGS -I${PREFIX}/src"
CFLAGS="$CFLAGS $(sdl2-config --cflags)"
CFLAGS="$CFLAGS $(pkg-config --cflags freetype2)"

CFLAGS="$CFLAGS -Wall -Wextra"
CFLAGS="$CFLAGS -Wundef"
CFLAGS="$CFLAGS -Wcast-align"
CFLAGS="$CFLAGS -Wwrite-strings"
CFLAGS="$CFLAGS -Wunreachable-code"
CFLAGS="$CFLAGS -Wformat=2"
CFLAGS="$CFLAGS -Wswitch-enum"
CFLAGS="$CFLAGS -Winit-self"
CFLAGS="$CFLAGS -Wno-implicit-fallthrough"

LIBS="liblegion.a"
LIBS="$LIBS $(sdl2-config --libs)"
LIBS="$LIBS $(pkg-config --libs freetype2)"

parallel $CC -c -o "{}.o" "${PREFIX}/src/{}.c" $CFLAGS ::: ${SRC[@]}

OBJ=""
for obj in "${SRC[@]}"; do OBJ="$OBJ ${obj}.o"; done
ar rcs liblegion.a $OBJ

$CC -o "legion" "${PREFIX}/src/main.c" $LIBS $CFLAGS
cp -r "${PREFIX}/res" .

parallel $CC -o "test_{}" "${PREFIX}/test/{}_test.c" $LIBS $CFLAGS ::: ${TEST[@]}
for test in "${TEST[@]}"; do
    echo "testing ${test}..."; "./test_$test" "${PREFIX}"
done
