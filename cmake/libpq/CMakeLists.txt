# libpq — custom CMake build из external_sources/libpq (postgresql-18.3 source)
#
# Заменяет meson+ninja (Windows) и autoconf+make (Linux). Никакого Python/meson/
# bison/flex/perl/autoconf на сборочной машине пользователя.
#
# Generated headers лежат в generated/ (Windows) и generated-linux/ (Linux) —
# регенерятся через bootstrap-generated.bat / bootstrap-libpq-linux.sh
# когда апгрейдим external_sources/libpq (developer one-time step).
#
# Что включено / выключено:
#   - SSL: Windows — нет (curl сам через Schannel), Linux — OpenSSL static
#   - GSSAPI / LDAP / OAuth / NLS / SSPI / zlib — НЕТ на обеих платформах
#
# Что отдаём наружу:
#   - target `libpq` — SHARED .dll (Windows) / .so.5 (Linux)
#   - alias `PostgreSQL::PostgreSQL` (совместимо со стандартным find_package)

cmake_minimum_required(VERSION 3.20)
project(libpq_local LANGUAGES C)

set(LIBPQ_SRC "${CMAKE_SOURCE_DIR}/external_sources/libpq")

# ============================================================
# Platform-specific paths и source lists
# ============================================================
if(WIN32)
    set(LIBPQ_GEN "${CMAKE_CURRENT_SOURCE_DIR}/generated")
elseif(UNIX)
    set(LIBPQ_GEN "${CMAKE_CURRENT_SOURCE_DIR}/generated-linux")
    if(NOT EXISTS "${LIBPQ_GEN}/include/pg_config.h")
        message(FATAL_ERROR
            "cmake/libpq/generated-linux/include/pg_config.h отсутствует. "
            "Это pre-generated артефакт, собирается через "
            "bash cmake/scripts/bootstrap-libpq-linux.sh (нужен Docker). "
            "После сборки закоммитить cmake/libpq/generated-linux/.")
    endif()
endif()

# ============================================================
# pgport — platform abstraction
# Из src/port/meson.build: pgport_sources + Windows-specific (или POSIX subset)
# + CRC32C runtime check (SSE42 + sb8 software fallback)
# ============================================================
set(PGPORT_SOURCES_COMMON
    # base — нужны на всех платформах
    src/port/bsearch_arg.c
    src/port/chklocale.c
    src/port/inet_net_ntop.c
    src/port/noblock.c
    src/port/path.c
    src/port/pg_bitutils.c
    src/port/pg_localeconv_r.c
    src/port/pg_numa.c
    src/port/pg_popcount_avx512.c
    src/port/pg_strong_random.c
    src/port/pgcheckdir.c
    src/port/pgmkdirp.c
    src/port/pgsleep.c
    src/port/pgstrcasecmp.c
    src/port/pgstrsignal.c
    src/port/pqsignal.c
    src/port/qsort.c
    src/port/qsort_arg.c
    src/port/quotes.c
    src/port/snprintf.c
    src/port/strerror.c
    src/port/tar.c
    # CRC32C runtime check (SSE42 + sb8 software fallback)
    src/port/pg_crc32c_sse42.c
    src/port/pg_crc32c_sse42_choose.c
    src/port/pg_crc32c_sb8.c
)

if(WIN32)
    set(PGPORT_SOURCES
        ${PGPORT_SOURCES_COMMON}
        # Windows-specific
        src/port/dirmod.c
        src/port/kill.c
        src/port/open.c
        src/port/system.c
        src/port/win32common.c
        src/port/win32dlopen.c
        src/port/win32env.c
        src/port/win32error.c
        src/port/win32fdatasync.c
        src/port/win32fseek.c
        src/port/win32gai_strerror.c
        src/port/win32getrusage.c
        src/port/win32link.c
        src/port/win32ntdll.c
        src/port/win32pread.c
        src/port/win32pwrite.c
        src/port/win32security.c
        src/port/win32setlocale.c
        src/port/win32stat.c
        # MSVC-specific (cc.get_id() == 'msvc')
        src/port/dirent.c
        src/port/win32gettimeofday.c
        # replace_funcs_neg для Windows (нет в MSVC CRT)
        src/port/explicit_bzero.c
        src/port/getopt.c
        src/port/getopt_long.c
        src/port/getpeereid.c
        src/port/inet_aton.c
        src/port/mkdtemp.c
        src/port/strlcat.c
        src/port/strlcpy.c
        src/port/strsep.c
        src/port/timingsafe_bcmp.c
    )
elseif(UNIX)
    set(PGPORT_SOURCES
        ${PGPORT_SOURCES_COMMON}
        # POSIX replace_funcs_neg — glibc не имеет strlcat/strlcpy/timingsafe_bcmp/getpeereid,
        # остальные есть. explicit_bzero — есть с glibc 2.25+, не включаем.
        # getpeereid — BSD, в glibc нет; PG-реализация через SO_PEERCRED getsockopt.
        src/port/getpeereid.c
        src/port/strlcat.c
        src/port/strlcpy.c
        src/port/timingsafe_bcmp.c
    )
endif()

# ============================================================
# pgcommon — shared frontend library
# Из src/common/meson.build: common_sources + crypto (OpenSSL или builtin)
# + ryu (d2s/f2s) + frontend extras
# ============================================================
set(PGCOMMON_SOURCES
    src/common/archive.c
    src/common/base64.c
    src/common/binaryheap.c
    src/common/blkreftable.c
    src/common/checksum_helper.c
    src/common/compression.c
    src/common/controldata_utils.c
    src/common/encnames.c
    src/common/exec.c
    src/common/file_perm.c
    src/common/file_utils.c
    src/common/hashfn.c
    src/common/ip.c
    src/common/jsonapi.c
    src/common/keywords.c
    src/common/kwlookup.c
    src/common/link-canary.c
    src/common/md5_common.c
    src/common/parse_manifest.c
    src/common/percentrepl.c
    src/common/pg_get_line.c
    src/common/pg_lzcompress.c
    src/common/pg_prng.c
    src/common/pgfnames.c
    src/common/psprintf.c
    src/common/relpath.c
    src/common/rmtree.c
    src/common/saslprep.c
    src/common/scram-common.c
    src/common/string.c
    src/common/stringinfo.c
    src/common/unicode_case.c
    src/common/unicode_category.c
    src/common/unicode_norm.c
    src/common/username.c
    src/common/wait_error.c
    src/common/wchar.c
    # ryu (double/float to string)
    src/common/d2s.c
    src/common/f2s.c
    # frontend shlib extras
    src/common/restricted_token.c
    src/common/sprompt.c
    # fe_memutils.c + logging.c — meson исключает их из shlib "по политике"
    # (libpq не должен exit() на OOM), но физически без них линкуется куча
    # неразрешённых palloc/pfree/pstrdup/pg_log_generic. Включаем — это даст
    # рабочий libpq, поведение OOM немного hard-fail но это приемлемо.
    src/common/fe_memutils.c
    src/common/logging.c
)

if(WIN32)
    # Windows без OpenSSL — собственные крипто-реализации
    list(APPEND PGCOMMON_SOURCES
        src/common/cryptohash.c
        src/common/hmac.c
        src/common/md5.c
        src/common/sha1.c
        src/common/sha2.c
    )
elseif(UNIX)
    # Linux с OpenSSL — крипто через OpenSSL backend (вместо builtin)
    list(APPEND PGCOMMON_SOURCES
        src/common/cryptohash_openssl.c
        src/common/hmac_openssl.c
        # md5/sha1/sha2 — есть в OpenSSL, builtin не нужны
        # Однако `md5_common.c` (выше) — это shared helper, не дубль
    )
endif()

# ============================================================
# libpq — собственно client library
# Из src/interfaces/libpq/meson.build: libpq_sources + Windows / POSIX
# ============================================================
set(LIBPQ_SOURCES
    src/interfaces/libpq/fe-auth-oauth.c
    src/interfaces/libpq/fe-auth-scram.c
    src/interfaces/libpq/fe-auth.c
    src/interfaces/libpq/fe-cancel.c
    src/interfaces/libpq/fe-connect.c
    src/interfaces/libpq/fe-exec.c
    src/interfaces/libpq/fe-lobj.c
    src/interfaces/libpq/fe-misc.c
    src/interfaces/libpq/fe-print.c
    src/interfaces/libpq/fe-protocol3.c
    src/interfaces/libpq/fe-secure.c
    src/interfaces/libpq/fe-trace.c
    src/interfaces/libpq/legacy-pqsignal.c
    src/interfaces/libpq/libpq-events.c
    src/interfaces/libpq/pqexpbuffer.c
)

if(WIN32)
    list(APPEND LIBPQ_SOURCES
        src/interfaces/libpq/pthread-win32.c
        src/interfaces/libpq/win32.c
    )
elseif(UNIX)
    # SSL через OpenSSL — fe-secure-{common,openssl}.c обязательны при USE_OPENSSL
    list(APPEND LIBPQ_SOURCES
        src/interfaces/libpq/fe-secure-common.c
        src/interfaces/libpq/fe-secure-openssl.c
    )
endif()

list(TRANSFORM PGPORT_SOURCES PREPEND "${LIBPQ_SRC}/")
list(TRANSFORM PGCOMMON_SOURCES PREPEND "${LIBPQ_SRC}/")
list(TRANSFORM LIBPQ_SOURCES PREPEND "${LIBPQ_SRC}/")

# ============================================================
# Target
# ============================================================
add_library(libpq SHARED
    ${LIBPQ_SOURCES}
    ${PGCOMMON_SOURCES}
    ${PGPORT_SOURCES}
)

if(WIN32)
    set_target_properties(libpq PROPERTIES
        OUTPUT_NAME "libpq"
        PREFIX ""
        RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        # postgres source не использует __declspec(dllexport) для PQ-функций.
        # На meson-сборке этот вопрос решается через .def файл сгенеренный perl-скриптом
        # из src/interfaces/libpq/exports.txt. Чтобы не дублировать perl-логику —
        # включаем WINDOWS_EXPORT_ALL_SYMBOLS: link.exe сам сделает .def со всеми
        # non-static функциями. Получится больше exports чем нужно, но безопасно.
        WINDOWS_EXPORT_ALL_SYMBOLS  TRUE
    )
elseif(UNIX)
    # На Linux у libpq стандартное имя `libpq.so.5` — SONAME важен потому что
    # бинарь VTL через DT_NEEDED ищет ровно это имя.
    set_target_properties(libpq PROPERTIES
        OUTPUT_NAME    "pq"      # libCMAKE даст префикс "lib" → libpq
        VERSION        "5.16"    # → libpq.so.5.16
        SOVERSION      "5"       # → libpq.so.5 (symlink)
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endif()

# ============================================================
# Include directories
# ============================================================
target_include_directories(libpq
    PUBLIC
        "${LIBPQ_SRC}/src/interfaces/libpq"
        "${LIBPQ_SRC}/src/include"
    PRIVATE
        "${LIBPQ_GEN}/include"
        "${LIBPQ_GEN}/common"
        "${LIBPQ_SRC}/src/port"
)

if(WIN32)
    target_include_directories(libpq PRIVATE
        "${LIBPQ_SRC}/src/include/port/win32"
        "${LIBPQ_SRC}/src/include/port/win32_msvc"
    )
endif()

# ============================================================
# Compile definitions
# ============================================================
# FRONTEND — режим клиента (без серверных глобалов).
# SO_MAJOR_VERSION — для libpq символьной версии.
# USE_DYNAMIC_OAUTH — заглушка (libpq может работать без libpq-oauth shared module).
# CRC32C runtime check (детектит SSE42 на старте — если нет, fallback на sb8).
target_compile_definitions(libpq PRIVATE
    FRONTEND
    SO_MAJOR_VERSION=5
    USE_DYNAMIC_OAUTH
    USE_SSE42_CRC32C_WITH_RUNTIME_CHECK
)

if(WIN32)
    target_compile_definitions(libpq PRIVATE
        WIN32
        _WINDOWS
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_DEPRECATE
    )
elseif(UNIX)
    # _GNU_SOURCE — для glibc-specific функций (strsignal, asprintf, и т.п.)
    # USE_OPENSSL — включает fe-secure-openssl.c, cryptohash_openssl.c, hmac_openssl.c
    target_compile_definitions(libpq PRIVATE
        _GNU_SOURCE
        USE_OPENSSL
    )
endif()

# ============================================================
# Compile options (warnings)
# ============================================================
if(MSVC)
    target_compile_options(libpq PRIVATE /W1 /wd4018 /wd4244 /wd4267 /wd4273)
elseif(UNIX)
    # postgres source не чистый по нашим стандартам — глушим warning'и в нём
    target_compile_options(libpq PRIVATE
        -w
        -fPIC
        -fno-strict-aliasing
        -fwrapv
    )
endif()

# ============================================================
# Link libraries
# ============================================================
if(WIN32)
    # Windows system libs:
    #   ws2_32 — TCP sockets
    #   secur32 — SSPI auth fallback
    #   advapi32 — реестр для service detection
    #   shell32 — SHGetFolderPath
    #   crypt32 — cryptohash через CryptoAPI
    #   netapi32 — NetUserGetInfo (chklocale)
    #   wldap32 — даже без LDAP win32error.c может тянуть строки
    target_link_libraries(libpq PRIVATE
        ws2_32 secur32 advapi32 shell32 crypt32 netapi32 wldap32
    )
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(libpq PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
        ${CMAKE_DL_LIBS}   # libdl для dlopen в pgcommon
        m                  # libm для math (pg_lzcompress, etc)
    )
endif()

# Public alias для совместимости со стандартным find_package(PostgreSQL)
add_library(PostgreSQL::PostgreSQL ALIAS libpq)
