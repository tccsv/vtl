#!/bin/sh
# Заглушка для PG ./configure version-check на bison.
#
# pg configure при сборке libpq frontend проверяет "bison --version" но
# фактически не вызывает bison — он нужен только для backend gram.y/scan.l.
# Скрипт выдаёт правдоподобную версию на --version, no-op на остальное.
#
# Используется в cmake/Dependencies-Linux.cmake через BISON=<path>/fake-bison.sh
# чтобы не требовать apt install bison flex на сборочной машине.

case "$1" in
    --version|-V)
        echo "bison (GNU Bison) 3.8.2"
        echo "Copyright (C) 2021 Free Software Foundation, Inc."
        exit 0
        ;;
    *)
        # любой другой вызов — должен означать что bison реально нужен,
        # но для libpq frontend этого не произойдёт. Падаем громко чтобы
        # ловить регрессии (вдруг pg вдруг начнёт реально звать bison).
        echo "fake-bison: вызван с аргументами '$*' — bison реально нужен, добавьте его в Dockerfile" >&2
        exit 1
        ;;
esac
