# Минимальная среда для проверки "собирается ли VTL на чистой Linux-системе".
# Аналог "чистая Windows + VS Community + CMake" из требований препода.
#
# Запуск из корня репо:
#   docker build -t vtl-clean -f tools/linux-clean-build/Dockerfile .
#
# Внутри установлен только физический минимум для C+CMake-сборки:
#   gcc, g++, make, cmake, libc6-dev — компилятор, линкер, headers libc.
# Никаких git/wget/curl/pkg-config/python/perl/nasm/autoconf — ровно ноль.

FROM debian:bookworm-slim

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        gcc \
        g++ \
        make \
        cmake \
        libc6-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /vtl
COPY . /vtl

# Делаем shell-скрипты исполняемыми — git под Windows может терять +x bit.
RUN chmod +x cmake/scripts/*.sh

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build -j"$(nproc)"
