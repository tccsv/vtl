# Linux: всё из source через external_sources/, либо pre-built static .a из external_libs/.
# На сборочной машине нужен ТОЛЬКО: gcc, g++, make, cmake, libc6-dev. Больше ничего.
#
# Раскладка по компонентам:
#   OpenSSL → external_libs/openssl/lib/lib{ssl,crypto}.a        — pre-built static (build-openssl-linux.sh)
#   curl    → add_subdirectory(external_sources/curl)            — CMake, с нашим OpenSSL, без zlib/brotli/zstd
#   libpq   → ExternalProject_Add(external_sources/libpq)        — pg-родное ./configure && make src/interfaces/libpq
#   FFmpeg  → ExternalProject_Add(external_sources/ffmpeg)       — ffmpeg ./configure --disable-asm --disable-everything --enable-...
#
# Финальный layout app/:
#   VTL  + libpq.so.5  + libavcodec.so + libavformat.so + libavutil.so + libavfilter.so + libswresample.so + libswscale.so
# curl линкуется статически (внутри VTL).
# zlib в VTL не используется — отключён в curl/libpq/ffmpeg.

include(ExternalProject)
include(ProcessorCount)
ProcessorCount(_VTL_NPROC)
if(_VTL_NPROC EQUAL 0)
    set(_VTL_NPROC 2)
endif()

# Threads::Threads нужен в transitive_link FFmpeg static — поднимаем find_package
# сюда (раньше он был только в корневом CMakeLists.txt после include Dependencies).
find_package(Threads REQUIRED)

# ============================================================
# OpenSSL — pre-built static .a из external_libs/openssl/
# ============================================================
# Собирается developer'ом через cmake/scripts/build-openssl-linux.sh (Docker).
# Финальная сборка пользователя/препода — только линковка с уже готовыми .a.
#
# Используем стандартный find_package(OpenSSL) — он сам создаст OpenSSL::SSL
# и OpenSSL::Crypto как GLOBAL IMPORTED targets, видимые внутри try_compile
# (curl это требует). OPENSSL_ROOT_DIR указывает на наш каталог — find_package
# не лезет в систему.
set(OPENSSL_ROOT "${EXTERNAL_LIBS_DIR}/openssl")

if(NOT EXISTS "${OPENSSL_ROOT}/lib/libssl.a")
    message(FATAL_ERROR
        "external_libs/openssl/lib/libssl.a отсутствует. "
        "Это pre-built артефакт, собирается через "
        "bash cmake/scripts/build-openssl-linux.sh (нужен Docker). "
        "После сборки закоммитить external_libs/openssl/.")
endif()

set(OPENSSL_ROOT_DIR        "${OPENSSL_ROOT}" CACHE PATH "" FORCE)
set(OPENSSL_USE_STATIC_LIBS TRUE CACHE BOOL "" FORCE)
# Прямые подсказки FindOpenSSL — гарантируем что не уйдёт искать в систему
set(OPENSSL_INCLUDE_DIR     "${OPENSSL_ROOT}/include" CACHE PATH "" FORCE)
set(OPENSSL_CRYPTO_LIBRARY  "${OPENSSL_ROOT}/lib/libcrypto.a" CACHE FILEPATH "" FORCE)
set(OPENSSL_SSL_LIBRARY     "${OPENSSL_ROOT}/lib/libssl.a" CACHE FILEPATH "" FORCE)

find_package(OpenSSL 3.0 REQUIRED)

# Проверка что подхватили нашу копию, а не системную (например libssl3 из Debian)
get_target_property(_openssl_loc OpenSSL::SSL IMPORTED_LOCATION)
if(NOT _openssl_loc MATCHES "^${OPENSSL_ROOT}")
    message(FATAL_ERROR
        "find_package(OpenSSL) подхватил библиотеку не из external_libs/openssl/: ${_openssl_loc}. "
        "Это нарушает требование 'все либы из проекта'. Проверь OPENSSL_ROOT_DIR.")
endif()
message(STATUS "OpenSSL: ${_openssl_loc} (project-local pre-built)")

# ============================================================
# curl — static из external_sources/curl через add_subdirectory
# ============================================================
# Опции CURL_* перебиты до add_subdirectory чтобы её собственный CMakeLists
# подхватил статический режим, OpenSSL backend и отключил всё лишнее.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON  CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH OFF CACHE BOOL "" FORCE)
set(CURL_USE_GSSAPI OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
set(USE_LIBRTMP OFF CACHE BOOL "" FORCE)
set(ENABLE_UNIX_SOCKETS ON CACHE BOOL "" FORCE)
set(HTTP_ONLY OFF CACHE BOOL "" FORCE) # нам нужны FTP/SMTP? оставлю полный

add_subdirectory("${EXTERNAL_SOURCES_DIR}/curl" "${CMAKE_BINARY_DIR}/curl-build" EXCLUDE_FROM_ALL)

# curl CMake уже создаёт CURL::libcurl target (alias на static при BUILD_STATIC_LIBS=ON).
# Не дублируем — VTL_utils линкуется напрямую через CURL::libcurl.

# ============================================================
# libpq — наш custom CMake build из external_sources/libpq
# ============================================================
# Аналог Windows подхода: source list собирается вручную, pre-generated
# headers лежат в cmake/libpq/generated-linux/ (bootstrap-libpq-linux.sh
# их пересоздаёт когда апгрейдим PG source).
# Так не требуется bison/flex/perl/autoconf на сборочной машине пользователя.
add_subdirectory("${CMAKE_SOURCE_DIR}/cmake/libpq" "${CMAKE_BINARY_DIR}/libpq-build")

# ============================================================
# FFmpeg — ffmpeg ./configure && make через ExternalProject_Add
# ============================================================
# --disable-asm: не нужен nasm/yasm на сборочной машине (медленнее на 20-30%, но zero-extra-tools).
# --disable-everything: отключаем всё, явно включаем только то что использует VTL.
# Минимальный набор decoder/encoder/muxer/demuxer/filter под текущее использование (text/audio/video pipelines).
set(FFMPEG_SRC      "${EXTERNAL_SOURCES_DIR}/ffmpeg")
set(FFMPEG_BUILD    "${CMAKE_BINARY_DIR}/ffmpeg-build")
set(FFMPEG_INSTALL  "${CMAKE_BINARY_DIR}/ffmpeg-install")
set(FFMPEG_INCLUDE  "${FFMPEG_INSTALL}/include")
set(FFMPEG_LIB      "${FFMPEG_INSTALL}/lib")

# Static .a — линкуются прямо в VTL exe, никаких .so в app/, никакого patchelf.
set(FFMPEG_A_AVUTIL      "${FFMPEG_LIB}/libavutil.a")
set(FFMPEG_A_AVCODEC     "${FFMPEG_LIB}/libavcodec.a")
set(FFMPEG_A_AVFORMAT    "${FFMPEG_LIB}/libavformat.a")
set(FFMPEG_A_AVFILTER    "${FFMPEG_LIB}/libavfilter.a")
set(FFMPEG_A_SWSCALE     "${FFMPEG_LIB}/libswscale.a")
set(FFMPEG_A_SWRESAMPLE  "${FFMPEG_LIB}/libswresample.a")

ExternalProject_Add(ffmpeg_external
    SOURCE_DIR        "${FFMPEG_SRC}"
    PREFIX            "${FFMPEG_BUILD}"
    BINARY_DIR        "${FFMPEG_BUILD}/build"
    INSTALL_DIR       "${FFMPEG_INSTALL}"
    DOWNLOAD_COMMAND  ""
    UPDATE_COMMAND    ""
    CONFIGURE_COMMAND
        "${FFMPEG_SRC}/configure"
            --prefix=${FFMPEG_INSTALL}
            --enable-static
            --disable-shared
            --enable-pic
            --disable-asm
            --disable-doc
            --disable-programs
            --disable-debug
            --disable-everything
            --disable-network
            --disable-autodetect
            # decoders
            --enable-decoder=h264
            --enable-decoder=hevc
            --enable-decoder=mpeg4
            --enable-decoder=aac
            --enable-decoder=mp3
            --enable-decoder=opus
            --enable-decoder=vorbis
            --enable-decoder=flac
            --enable-decoder=pcm_s16le
            --enable-decoder=pcm_s24le
            --enable-decoder=pcm_f32le
            --enable-decoder=mjpeg
            --enable-decoder=png
            --enable-decoder=bmp
            --enable-decoder=ass
            --enable-decoder=srt
            --enable-decoder=subrip
            --enable-decoder=webvtt
            # encoders
            --enable-encoder=h264
            --enable-encoder=mpeg4
            --enable-encoder=aac
            --enable-encoder=mp3
            --enable-encoder=opus
            --enable-encoder=flac
            --enable-encoder=pcm_s16le
            --enable-encoder=mjpeg
            --enable-encoder=png
            --enable-encoder=bmp
            --enable-encoder=ass
            --enable-encoder=srt
            --enable-encoder=subrip
            --enable-encoder=webvtt
            # muxers / demuxers
            --enable-demuxer=mov
            --enable-demuxer=mp4
            --enable-demuxer=matroska
            --enable-demuxer=mp3
            --enable-demuxer=wav
            --enable-demuxer=flac
            --enable-demuxer=aac
            --enable-demuxer=ogg
            --enable-demuxer=ass
            --enable-demuxer=srt
            --enable-demuxer=webvtt
            --enable-muxer=mov
            --enable-muxer=mp4
            --enable-muxer=matroska
            --enable-muxer=mp3
            --enable-muxer=wav
            --enable-muxer=flac
            --enable-muxer=ass
            --enable-muxer=srt
            --enable-muxer=webvtt
            --enable-muxer=ipod
            # parsers
            --enable-parser=h264
            --enable-parser=hevc
            --enable-parser=aac
            --enable-parser=mpeg4video
            # protocols
            --enable-protocol=file
            --enable-protocol=pipe
            # filters (мало — субтитры через libass отключены)
            --enable-filter=null
            --enable-filter=anull
            --enable-filter=aresample
            --enable-filter=scale
            --enable-filter=copy
            --enable-filter=format
            --enable-filter=aformat
            --enable-filter=trim
            --enable-filter=atrim
            --enable-filter=concat
    BUILD_COMMAND     $(MAKE) -j${_VTL_NPROC}
    INSTALL_COMMAND   $(MAKE) install
    BUILD_BYPRODUCTS
        "${FFMPEG_A_AVUTIL}" "${FFMPEG_A_AVCODEC}" "${FFMPEG_A_AVFORMAT}"
        "${FFMPEG_A_AVFILTER}" "${FFMPEG_A_SWSCALE}" "${FFMPEG_A_SWRESAMPLE}"
    LOG_CONFIGURE     OFF
    LOG_BUILD         OFF
    LOG_INSTALL       OFF
)

file(MAKE_DIRECTORY "${FFMPEG_INCLUDE}")

# Порядок важен: linker идёт слева направо, разрешая символы. avformat зависит
# от avcodec, тот от avutil. Утилитные (swscale/swresample) — после avcodec.
foreach(_comp avformat avcodec avfilter swscale swresample avutil)
    add_library(ffmpeg_${_comp} STATIC IMPORTED GLOBAL)
    set_target_properties(ffmpeg_${_comp} PROPERTIES
        IMPORTED_LOCATION             "${FFMPEG_LIB}/lib${_comp}.a"
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE}"
    )
    add_dependencies(ffmpeg_${_comp} ffmpeg_external)
endforeach()

add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE
    ffmpeg_avformat ffmpeg_avcodec ffmpeg_avfilter
    ffmpeg_swscale ffmpeg_swresample ffmpeg_avutil
    # Транзитивные глибс-симвoлы для FFmpeg static: pthread, dl, math, rt
    Threads::Threads ${CMAKE_DL_LIBS} m rt
)

# ============================================================
# RPATH: app/VTL должен находить libpq.so.5 и FFmpeg .so в runtime
# ============================================================
# libpq.so.5 — наш target с LIBRARY_OUTPUT_DIRECTORY=app/.
# FFmpeg .so — копируются tar'ом в app/ через POST_BUILD корневого CMakeLists.
# $ORIGIN — относительный путь от exe (т.е. app/).
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN")
set(CMAKE_BUILD_RPATH "\$ORIGIN")

message(STATUS "VTL Linux deps: source-build (curl static, libpq custom CMake, ffmpeg ExternalProject, OpenSSL pre-built static)")
