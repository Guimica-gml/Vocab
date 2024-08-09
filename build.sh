#!/usr/bin/env sh
set -e

CFLAGS="-Wall -Wextra -pedantic -ggdb -std=c11"
CLIBS="-lraylib"

gcc $CFLAGS -o vocab main.c $CLIBS

if [ "$1" = "run" ]
then
    shift
    ./vocab "$@"
fi
