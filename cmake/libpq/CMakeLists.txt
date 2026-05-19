# libpq — custom CMake build из external_sources/libpq (postgresql-18.3 source)
#
# Заменяет meson+ninja цепочку. Никакого Python/meson на сборочной машине.
# Generated headers (kwlist_d.h, pg_config.h, pg_config_os.h, pg_config_paths.h)
# лежат в generated/ — это вывод meson однократно зафиксированный в репо
# (аналогично тому как ShiftMediaProject держит config.h для FFmpeg).
#
# Что включено / выключено:
#   - SSL/GSSAPI/LDAP/OAuth — НЕТ (требуют OpenSSL/MIT Kerberos/libcurl)
#   - NLS — НЕТ (не нужен gettext)
#   - SSPI — НЕТ (упрощение)
#   - zlib — НЕТ
#
# Что отдаём наружу:
#   - target `libpq` — SHARED DLL
#   - alias `PostgreSQL::PostgreSQL` (совместимо со стандартным find_package)
#   - include path: src/interfaces/libpq + src/include

cmake_minimum_required(VERSION 3.20)
project(libpq_local LANGUAGES C)

set(LIBPQ_SRC "${CMAKE_SOURCE_DIR}/external_sources/libpq")
set(LIBPQ_GEN "${CMAKE_CURRENT_SOURCE_DIR}/generated")

# ============================================================
# pgport — platform abstraction (Windows/MSVC subset)
# Из src/port/meson.build: pgport_sources + Windows + MSVC + replace_funcs_neg
# (все nego: которых нет в MSVC CRT) + CRC32C runtime check (SSE42 + sb8 fallback)
# ============================================================
set(PGPORT_SOURCES
    # base
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
    # replace_funcs_neg (нет в MSVC CRT)
    src/port/explicit_bzero.c
    src/port/getopt.c
    src/port/getopt_long.c
    src/port/getpeereid.c
    src/port/inet_aton.c
    src/port/mkdtemp.c
    src/port/strlcat.c
    src/port/strlcpy.c
    # strnlen.c — НЕ включаем: MSVC UCRT уже экспортирует strnlen
    # (pg_config.h должен иметь HAVE_STRNLEN=1 — meson так и определил)
    src/port/strsep.c
    src/port/timingsafe_bcmp.c
    # CRC32C — runtime check (SSE42 + sb8 software fallback)
    src/port/pg_crc32c_sse42.c
    src/port/pg_crc32c_sse42_choose.c
    src/port/pg_crc32c_sb8.c
)

# ============================================================
# pgcommon — shared frontend library
# Из src/common/meson.build: common_sources + (no ssl: cryptohash/hmac/md5/sha1/sha2)
# + ryu (d2s/f2s) + frontend extras (restricted_token/sprompt)
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
    # crypto без OpenSSL — собственные реализации
    src/common/cryptohash.c
    src/common/hmac.c
    src/common/md5.c
    src/common/sha1.c
    src/common/sha2.c
    # frontend shlib extras
    src/common/restricted_token.c
    src/common/sprompt.c
    # fe_memutils.c + logging.c — meson исключает их из shlib "по политике"
    # (libpq не должен exit() на OOM), но физически без них линкуется куча
    # неразрешённых palloc/pfree/pstrdup/pg_log_generic. Включаем — это даст
    # рабочий libpq.dll, поведение OOM немного hard-fail но это приемлемо.
    src/common/fe_memutils.c
    src/common/logging.c
)

# ============================================================
# libpq — собственно client library
# Из src/interfaces/libpq/meson.build: libpq_sources + Windows + (no ssl/gssapi)
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
    src/interfaces/libpq/pthread-win32.c
    src/interfaces/libpq/win32.c
)

list(TRANSFORM PGPORT_SOURCES PREPEND "${LIBPQ_SRC}/")
list(TRANSFORM PGCOMMON_SOURCES PREPEND "${LIBPQ_SRC}/")
list(TRANSFORM LIBPQ_SOURCES PREPEND "${LIBPQ_SRC}/")

add_library(libpq SHARED
    ${LIBPQ_SOURCES}
    ${PGCOMMON_SOURCES}
    ${PGPORT_SOURCES}
)

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
    # non-static функциями. Получится больше exports чем нужно (попадут pgcommon/pgport),
    # но это безопасно и стабильно — VTL линкует только PQ-функции.
    WINDOWS_EXPORT_ALL_SYMBOLS  TRUE
)

target_include_directories(libpq
    PUBLIC
        "${LIBPQ_SRC}/src/interfaces/libpq"
        "${LIBPQ_SRC}/src/include"
    PRIVATE
        "${LIBPQ_GEN}/include"
        "${LIBPQ_GEN}/common"
        "${LIBPQ_SRC}/src/port"
        "${LIBPQ_SRC}/src/include/port/win32"
        "${LIBPQ_SRC}/src/include/port/win32_msvc"
)

# WIN32 / _WINDOWS — стандарт. FRONTEND — режим клиента (без серверных глобалов).
# SO_MAJOR_VERSION — для libpq символьной версии. USE_DYNAMIC_OAUTH — заглушка
# (libpq может работать без libpq-oauth shared module). CRC32C runtime check
# (детектит SSE42 на старте — если нет, fallback на sb8).
target_compile_definitions(libpq
    PRIVATE
        WIN32
        _WINDOWS
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_DEPRECATE
        FRONTEND
        SO_MAJOR_VERSION=5
        USE_DYNAMIC_OAUTH
        USE_SSE42_CRC32C_WITH_RUNTIME_CHECK
)

# Тихо предупреждения для FFmpeg-style source (postgres код предполагает warning level средний)
if(MSVC)
    target_compile_options(libpq PRIVATE /W1 /wd4018 /wd4244 /wd4267 /wd4273)
endif()

# Windows system libs нужны для:
#  ws2_32 — TCP sockets
#  secur32 — SSPI (используется при auth fallback)
#  advapi32 — реестр (для определения сервиса)
#  shell32 — SHGetFolderPath
#  crypt32 — для random/cert (cryptohash через CryptoAPI)
#  netapi32 — для NetUserGetInfo (chklocale)
#  wldap32 — даже без LDAP, win32error.c может тянуть строки
target_link_libraries(libpq PRIVATE
    ws2_32 secur32 advapi32 shell32 crypt32 netapi32 wldap32
)

# Public alias для совместимости со стандартным find_package(PostgreSQL)
add_library(PostgreSQL::PostgreSQL ALIAS libpq)
