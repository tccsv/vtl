#!/usr/bin/env bash
# Сборка OpenSSL 3.x static .a + headers для Linux source-build.
#
# Запускается ОДИН РАЗ разработчиком (или CI) при апгрейде версии OpenSSL.
# Результат — static .a + headers — коммитятся в external_libs/openssl/.
# Финальная сборка пользователя/препода эти .a уже не пересобирает.
#
# Аналог cmake/libpq/bootstrap-generated.bat — developer tool, не runtime.
#
# Запуск (из корня репо):
#   bash cmake/scripts/build-openssl-linux.sh
#
# Требования на машине разработчика: Docker. Больше ничего.
# Сборка идёт в debian:bookworm-slim контейнере чтобы glibc compat был
# минимальный — .a слинкуются с любым более новым glibc на стороне пользователя.

set -euo pipefail

OPENSSL_VERSION="${OPENSSL_VERSION:-3.0.17}"
REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUTPUT_DIR="${REPO_ROOT}/external_libs/openssl"

echo "[build-openssl] target version: ${OPENSSL_VERSION}"
echo "[build-openssl] output: ${OUTPUT_DIR}"

mkdir -p "${OUTPUT_DIR}/lib" "${OUTPUT_DIR}/include"

# Сборка внутри debian:bookworm-slim — glibc 2.36, gcc 12.
# no-shared: только .a файлы (статика, vendor-bundle в наш libpq.so / VTL exe)
# no-tests / no-docs / no-engine: ускоряем, нам не нужны
# enable-md4: нужно для libpq SCRAM auth (использует MD4)
docker run --rm \
    -v "${OUTPUT_DIR}:/output" \
    debian:bookworm-slim \
    bash -c "
        set -euo pipefail
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -qq
        apt-get install -y --no-install-recommends \
            build-essential perl wget ca-certificates >/dev/null
        cd /tmp
        wget -q https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz
        tar xzf openssl-${OPENSSL_VERSION}.tar.gz
        cd openssl-${OPENSSL_VERSION}
        ./Configure linux-x86_64 \
            no-shared no-tests no-engine no-legacy no-dso \
            -fPIC \
            --prefix=/openssl-install \
            --openssldir=/openssl-install/ssl
        make -j\$(nproc)
        make install_sw
        # lib64 на bookworm — стандартное расположение для 64-bit OpenSSL
        cp /openssl-install/lib64/libssl.a /output/lib/
        cp /openssl-install/lib64/libcrypto.a /output/lib/
        cp -r /openssl-install/include/openssl /output/include/
        # Strip debug symbols — уменьшаем размер коммита (10MB → ~5MB)
        strip --strip-debug /output/lib/libssl.a /output/lib/libcrypto.a
        chmod 644 /output/lib/*.a
        find /output/include -type f -exec chmod 644 {} \;
        echo
        echo 'sizes:'
        ls -la /output/lib/
    "

echo
echo "[build-openssl] done. Artifacts:"
echo "  ${OUTPUT_DIR}/lib/libssl.a"
echo "  ${OUTPUT_DIR}/lib/libcrypto.a"
echo "  ${OUTPUT_DIR}/include/openssl/*.h"
echo
echo "Не забудь:  git add external_libs/openssl/  &&  git commit -m 'build(openssl): refresh ${OPENSSL_VERSION}'"
