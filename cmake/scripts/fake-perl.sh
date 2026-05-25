#!/bin/sh
# Заглушка для PG ./configure version-check на perl.
#
# PG configure проверяет perl --version даже когда --without-perl
# (perl используется для генерации некоторых helper-скриптов).
# Для libpq frontend ни один из этих скриптов не нужен —
# stub отдаёт правдоподобную версию и no-op на остальное.

case "$1" in
    --version|-v|-V)
        echo "This is perl 5, version 36, subversion 0 (v5.36.0) built for x86_64-linux-gnu"
        echo ""
        echo "Copyright 1987-2022, Larry Wall"
        exit 0
        ;;
    *)
        echo "fake-perl: вызван с аргументами '$*' — perl реально нужен, добавьте его в Dockerfile" >&2
        exit 1
        ;;
esac
