#! /usr/bin/env bash

set -o errexit -o nounset -o pipefail

CC=${CC:-gcc}
: ${PREFIX:="."}
: ${VALGRIND:=""}
: ${PROFILE:=""}
: ${VERBOSE:=""}

if [ ! -z "${VERBOSE}" ]; then set -o xtrace; fi

declare -a SRC
SRC=(common items ui render game vm utils db)

declare -a TEST
: ${TEST:="ring text lisp chunk lanes tech save protocol items proxy man"}

TIMEFORMAT="%3R"
TIME="eval time"
ECHO="eval echo -n"
if [ -z "${PROFILE}" ]; then TIME=""; ECHO=":"; fi

CFLAGS="-ggdb -O3 -march=native -pipe -std=gnu11 -D_GNU_SOURCE -lm -pthread"
CFLAGS="$CFLAGS -I${PREFIX}/src"
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

LIBS="liblegion.a"
LIBS="$LIBS $(sdl2-config --libs)"
LIBS="$LIBS $(pkg-config --libs freetype2)"

if [ -f "./legion" ]; then
    $ECHO "db generation..."
    $TIME ./legion --db "${PREFIX}"
    rm "${PREFIX}"/src/db/gen/*.bak
fi

mkdir -p "obj"
$ECHO "object compilation..."
$TIME parallel $CC -c -o "obj/{}.o" "${PREFIX}/src/{}.c" $CFLAGS ::: ${SRC[@]}

OBJ=""
for obj in "${SRC[@]}"; do OBJ="$OBJ obj/${obj}.o"; done
ar rcs liblegion.a $OBJ

$ECHO "exec compilation..."
$TIME $CC -o "legion" "${PREFIX}/src/exec.c" $LIBS $CFLAGS
rm -r "./res"
cp -r "${PREFIX}/res" .

./legion --graph | dot -Tsvg > tapes.svg
./legion --items > items.lisp

mkdir -p "test"
$ECHO "test compilation..."
$TIME parallel $CC -o "test/{}" "${PREFIX}/test/{}_test.c" $LIBS $CFLAGS ::: ${TEST[@]}

$ECHO "test execution..."
if [ -z "${VALGRIND}" ]; then
    $TIME parallel "./test/{}" "${PREFIX}" ::: ${TEST[@]}
else
    $TIME parallel valgrind \
        --quiet \
        --leak-check=full \
        --track-origins=yes \
        --trace-children=yes \
        --error-exitcode=1 \
        --suppressions="${PREFIX}/legion.supp" \
        "./test/{}" "${PREFIX}" ::: ${TEST[@]}
fi
