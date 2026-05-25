#!/usr/bin/env bash
# Pre-generation PG headers для libpq Linux build БЕЗ bison/flex/perl на сборочной машине.
#
# Запускается ОДИН РАЗ разработчиком при апгрейде PG source.
# Результат — generated *.h файлы — коммитятся в cmake/libpq/generated-linux/.
# Финальная сборка пользователя/препода эти headers уже не пересоздаёт.
#
# Аналог cmake/libpq/bootstrap-generated.bat (Windows) — developer tool, не runtime.
#
# Запуск (из корня репо):
#   bash cmake/scripts/bootstrap-libpq-linux.sh
#
# Требования на машине разработчика: Docker. Больше ничего.
# Сборка идёт в debian:bookworm-slim с временной установкой bison/flex/perl
# (внутри Docker, на машину не тянется).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PG_SRC="${REPO_ROOT}/external_sources/libpq"
OUT_DIR="${REPO_ROOT}/cmake/libpq/generated-linux"

if [ ! -d "${PG_SRC}" ]; then
    echo "[bootstrap-libpq] PG source not found at ${PG_SRC}" >&2
    exit 1
fi

echo "[bootstrap-libpq] PG source: ${PG_SRC}"
echo "[bootstrap-libpq] output: ${OUT_DIR}"

mkdir -p "${OUT_DIR}/include" "${OUT_DIR}/common"

# Подсовываем нашу external_libs/openssl как системную через CFLAGS/LDFLAGS,
# чтобы --with-openssl нашёл libssl.a/libcrypto.a и в pg_config.h попал USE_OPENSSL=1
docker run --rm \
    -v "${PG_SRC}:/pg-src:ro" \
    -v "${OUT_DIR}:/output" \
    -v "${REPO_ROOT}/external_libs/openssl:/openssl:ro" \
    debian:bookworm-slim \
    bash -c '
        set -euo pipefail
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -qq
        apt-get install -y --no-install-recommends \
            build-essential bison flex perl pkg-config zlib1g-dev >/dev/null

        # Копируем PG source в writable место (configure пишет в source tree)
        cp -r /pg-src /pg
        cd /pg

        ./configure \
            --prefix=/pg-install \
            --without-readline \
            --without-zlib \
            --without-icu \
            --without-gssapi \
            --without-ldap \
            --without-llvm \
            --without-pam \
            --without-systemd \
            --without-bonjour \
            --without-selinux \
            --without-libxml \
            --without-libxslt \
            --without-perl \
            --without-python \
            --without-tcl \
            --with-openssl \
            --with-includes=/openssl/include \
            --with-libraries=/openssl/lib \
            CFLAGS="-O2 -fPIC" \
            >/tmp/configure.log 2>&1 || (tail -50 /tmp/configure.log && exit 1)

        # submake-generated-headers строит ВСЕ headers нужные backend и frontend
        # (errcodes.h, kwlist.h, lwlocknames.h, catalog/pg_*_d.h, fmgrtab/protos, jsonpath_gram.h, ...)
        make -j"$(nproc)" -C src/backend generated-headers
        make -j"$(nproc)" -C src/include
        make -j"$(nproc)" -C src/common

        # Скопировать в output: pg_config.h, pg_config_os.h, pg_config_paths.h
        # + все *_d.h из src/include/catalog/ (~70 файлов)
        # + lwlocknames.h, errcodes.h, kwlist.h, fmgrtab/protos/oids
        # + common/kwlist_d.h
        cp /pg/src/include/pg_config.h          /output/include/
        cp /pg/src/include/pg_config_ext.h      /output/include/ || true
        cp /pg/src/include/pg_config_os.h       /output/include/ || true
        cp /pg/src/include/pg_config_paths.h    /output/include/ || true

        mkdir -p /output/include/catalog /output/include/utils /output/include/parser /output/include/storage /output/include/nodes

        # catalog _d.h (~70 файлов)
        cp /pg/src/include/catalog/*_d.h /output/include/catalog/ 2>/dev/null || true

        # utils generated
        cp /pg/src/include/utils/errcodes.h     /output/include/utils/ 2>/dev/null || true
        cp /pg/src/include/utils/fmgroids.h     /output/include/utils/ 2>/dev/null || true
        cp /pg/src/include/utils/fmgrprotos.h   /output/include/utils/ 2>/dev/null || true
        cp /pg/src/backend/utils/fmgrtab.c      /output/include/utils/ 2>/dev/null || true
        cp /pg/src/include/utils/wait_event_types.h /output/include/utils/ 2>/dev/null || true
        cp /pg/src/include/utils/header-stamp   /output/include/utils/ 2>/dev/null || true
        cp /pg/src/include/utils/probes.h       /output/include/utils/ 2>/dev/null || true

        # parser/storage/nodes generated
        cp /pg/src/include/parser/kwlist_d.h    /output/include/parser/ 2>/dev/null || true
        cp /pg/src/include/storage/lwlocknames.h /output/include/storage/ 2>/dev/null || true
        cp /pg/src/include/nodes/header-stamp   /output/include/nodes/ 2>/dev/null || true

        # common generated
        cp /pg/src/common/kwlist_d.h            /output/common/ 2>/dev/null || true

        # ВСЕ оставшиеся generated .h из всего PG include tree (запасной grab-all)
        # Чтобы ничего не упустить — копируем всё что выше bind-mount источника по mtime
        find /pg/src/include -name "*.h" -newer /pg-src/configure -type f | while read f; do
            rel="${f#/pg/src/include/}"
            mkdir -p "/output/include/$(dirname "$rel")"
            cp "$f" "/output/include/$rel"
        done

        # Аналогично для common
        find /pg/src/common -name "*.h" -newer /pg-src/configure -type f | while read f; do
            rel="${f#/pg/src/common/}"
            mkdir -p "/output/common/$(dirname "$rel")"
            cp "$f" "/output/common/$rel"
        done

        chmod -R 644 /output/include/* /output/common/* 2>/dev/null || true
        find /output -type d -exec chmod 755 {} \;

        echo
        echo "[bootstrap-libpq] generated files:"
        find /output -type f | head -40
        echo "..."
        echo "[bootstrap-libpq] total: $(find /output -type f | wc -l) files"
    '

echo
echo "[bootstrap-libpq] done. Pre-generated headers в ${OUT_DIR}"
echo "Не забудь:  git add cmake/libpq/generated-linux/  &&  git commit -m 'build(libpq): bootstrap Linux headers'"
