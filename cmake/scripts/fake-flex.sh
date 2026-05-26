#!/bin/sh
# Заглушка для PG ./configure version-check на flex.
# Аналогична fake-bison.sh — flex нужен только для backend, libpq frontend
# не использует flex.

case "$1" in
    --version|-V)
        echo "flex 2.6.4"
        exit 0
        ;;
    *)
        echo "fake-flex: вызван с аргументами '$*' — flex реально нужен, добавьте его в Dockerfile" >&2
        exit 1
        ;;
esac
