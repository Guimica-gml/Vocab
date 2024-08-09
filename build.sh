#!/usr/bin/env sh
set -e

CFLAGS="-Wall -Wextra -pedantic -ggdb -std=c11"
CLIBS="-lraylib"

mkdir -p ./build
gcc $CFLAGS -o ./build/vocab main.c $CLIBS

if [ "$1" = "run" ]
then
    shift
    ./build/vocab "$@"
fi
