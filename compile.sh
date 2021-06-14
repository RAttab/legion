#! /usr/bin/env bash

set -o errexit -o nounset -o pipefail # -o xtrace

: ${PREFIX:="."}
: ${VM_DEBUG:=""}
: ${VALGRIND:=""}
: ${NK_REPO:="git@github.com:immediate-mode-ui/nuklear"}

declare -a SRC
SRC=(ui render game vm utils)

declare -a TEST
TEST=(coord vm)

CC=${CC:-gcc}

CFLAGS="-ggdb -O3 -march=native -pipe -std=gnu11 -D_GNU_SOURCE"
CFLAGS="$CFLAGS -I${PREFIX}/src"
CFLAGS="$CFLAGS -I${PREFIX}/build/nuklear"
CFLAGS="$CFLAGS $(sdl2-config --cflags)"
CFLAGS="$CFLAGS $(pkg-config --cflags freetype2)"

CFLAGS="$CFLAGS -Wall -Wextra"
CFLAGS="$CFLAGS -Wundef"
CFLAGS="$CFLAGS -Wcast-align"
CFLAGS="$CFLAGS -Wwrite-strings"
CFLAGS="$CFLAGS -Wunreachable-code"
CFLAGS="$CFLAGS -Wformat=2"
# CFLAGS="$CFLAGS -Wswitch-enum" // Sadly gets in the way alot
CFLAGS="$CFLAGS -Winit-self"
CFLAGS="$CFLAGS -Wno-implicit-fallthrough"
CFLAGS="$CFLAGS -Wno-address-of-packed-member" # very annoying
if [ ! -z "${VM_DEBUG}" ]; then CFLAGS="$CFLAGS -DVM_DEBUG"; fi

LIBS="liblegion.a"
LIBS="$LIBS $(sdl2-config --libs)"
LIBS="$LIBS $(pkg-config --libs freetype2)"

OBJ=""


## Nuklear

if [ ! -f "./nuklear.o" ]; then
    if [ ! -d "./nuklear" ]; then git clone --depth=1 "$NK_REPO" -- nuklear; fi
    ( cd "./nuklear/src"; ./paq.sh )
    NK_CFLAGS="-ggdb -O3 -march=native -pipe -std=gnu11 -D_GNU_SOURCE"
    NK_CFLAGS="$NK_CFLAGS -I${PREFIX}/src"
    NK_CFLAGS="$NK_CFLAGS -I${PREFIX}/build/nuklear"
    NK_CFLAGS="$NK_CFLAGS -Wno-unused-function"
    NK_CFLAGS="$NK_CFLAGS -Wno-unused-parameter"
    NK_CFLAGS="$NK_CFLAGS -Wno-maybe-uninitialized"
    $CC -c -o "nuklear.o"  "${PREFIX}/src/nk.c" $NK_CFLAGS
    OBJ="$OBJ nuklear.o"
fi


## Legion

parallel $CC -c -o "{}.o" "${PREFIX}/src/{}.c" $CFLAGS ::: ${SRC[@]}
for obj in "${SRC[@]}"; do OBJ="$OBJ ${obj}.o"; done
ar rcs liblegion.a $OBJ

$CC -o "legion" "${PREFIX}/src/main.c" $LIBS $CFLAGS
cp -r "${PREFIX}/res" .

parallel $CC -o "test_{}" "${PREFIX}/test/{}_test.c" $LIBS $CFLAGS ::: ${TEST[@]}

if [ -z "${VALGRIND}" ]; then
    parallel "./test_{}" "${PREFIX}" ::: ${TEST[@]}
else
    parallel valgrind \
        --leak-check=full \
        --track-origins=yes \
        --trace-children=yes \
        --error-exitcode=1 \
        "./test_{}" "${PREFIX}" ::: ${TEST[@]}
fi
