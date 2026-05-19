# Подключает зависимости из external_libs/ как IMPORTED CMake-таргеты.
# Никаких find_package / pkg-config — всё локально в репо.
# Цель: проект собирается на чистой ОС без apt/brew/vcpkg.
#
# Поддерживаемые платформы:
#   Linux x86_64      → external_libs/{ffmpeg,curl,postgresql}/lib/*.so
#   Windows x86_64    → собирается из external_sources/ под MSVC
#   macOS arm64       → external_libs/macos/lib/*.dylib

set(EXTERNAL_LIBS_DIR "${CMAKE_SOURCE_DIR}/external_libs")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include("${CMAKE_CURRENT_LIST_DIR}/Dependencies-Windows.cmake")
    return()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include("${CMAKE_CURRENT_LIST_DIR}/Dependencies-MacOS.cmake")
    return()
endif()

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(FATAL_ERROR
        "external_libs/ под ${CMAKE_SYSTEM_NAME} пока нет. "
        "Поддерживается Linux x86_64, Windows x86_64 (MSVC), macOS arm64.")
endif()

# ============================================================
# FFmpeg
# ============================================================
set(FFMPEG_LIB_DIR "${EXTERNAL_LIBS_DIR}/ffmpeg/lib")
set(FFMPEG_INC_DIR "${EXTERNAL_LIBS_DIR}/ffmpeg/include")

# Создаём недостающие файлы .so.MAJOR — физической копией .so.MAJOR.MINOR.PATCH.
# Они нужны для runtime: линкер записывает в DT_NEEDED soname библиотеки
# (например libavcodec.so.60), а в репо изначально лежит только версионированный
# файл. Симлинки тут не используем — на NTFS-маунтах (Docker, общая папка) они
# работают нестабильно. Файл-копия работает на любой ФС.
function(_vtl_ensure_soname dir versioned soname_name)
    set(target "${dir}/${soname_name}")
    if(NOT EXISTS "${target}")
        configure_file("${dir}/${versioned}" "${target}" COPYONLY)
    endif()
endfunction()

_vtl_ensure_soname("${FFMPEG_LIB_DIR}" "libavcodec.so.60.31.102"    "libavcodec.so.60")
_vtl_ensure_soname("${FFMPEG_LIB_DIR}" "libavformat.so.60.16.100"   "libavformat.so.60")
_vtl_ensure_soname("${FFMPEG_LIB_DIR}" "libavutil.so.58.29.100"     "libavutil.so.58")
_vtl_ensure_soname("${FFMPEG_LIB_DIR}" "libavfilter.so.9.12.100"    "libavfilter.so.9")
_vtl_ensure_soname("${FFMPEG_LIB_DIR}" "libswscale.so.7.5.100"      "libswscale.so.7")
_vtl_ensure_soname("${FFMPEG_LIB_DIR}" "libswresample.so.4.12.100"  "libswresample.so.4")

foreach(comp avcodec avformat avutil avfilter swscale swresample)
    add_library(ffmpeg_${comp} SHARED IMPORTED)
    set_target_properties(ffmpeg_${comp} PROPERTIES
        IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/lib${comp}.so"
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INC_DIR}"
    )
endforeach()

# Зонтичный таргет ffmpeg — модули проекта линкуются с ним
add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE
    ffmpeg_avcodec
    ffmpeg_avformat
    ffmpeg_avutil
    ffmpeg_avfilter
    ffmpeg_swscale
    ffmpeg_swresample
)

# ============================================================
# libcurl
# ============================================================
set(CURL_LIB_DIR "${EXTERNAL_LIBS_DIR}/curl/lib")
set(CURL_INC_DIR "${EXTERNAL_LIBS_DIR}/curl/include")

_vtl_ensure_soname("${CURL_LIB_DIR}" "libcurl.so.4.8.0" "libcurl.so.4")

add_library(CURL::libcurl SHARED IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LOCATION "${CURL_LIB_DIR}/libcurl.so.4.8.0"
    INTERFACE_INCLUDE_DIRECTORIES "${CURL_INC_DIR}"
)

# ============================================================
# libpq (PostgreSQL)
# ============================================================
set(PG_LIB_DIR "${EXTERNAL_LIBS_DIR}/postgresql/lib")
set(PG_INC_DIR "${EXTERNAL_LIBS_DIR}/postgresql/include")

_vtl_ensure_soname("${PG_LIB_DIR}" "libpq.so.5.16" "libpq.so.5")

add_library(PostgreSQL::PostgreSQL SHARED IMPORTED)
set_target_properties(PostgreSQL::PostgreSQL PROPERTIES
    IMPORTED_LOCATION "${PG_LIB_DIR}/libpq.so.5.16"
    INTERFACE_INCLUDE_DIRECTORIES "${PG_INC_DIR}"
)

# ============================================================
# Линкер: разрешить неразрешённые символы внутри shared libs
# ============================================================
# FFmpeg .so из external_libs собраны со многими опциональными кодеками
# (libaom, libdav1d, libopus, libmp3lame, libsnappy, libvidstab и др.).
# Эти transient .so есть на типовой Linux-системе с медиаподдержкой, но на
# чистом Ubuntu без них линкер падает с undefined reference, даже если
# наш код эти кодеки не вызывает.
#
# Флаг говорит линкеру: "не падай на неразрешённых символах *внутри shared libs*".
# Наши собственные неразрешённые символы по-прежнему ловятся.
# На runtime тот же ld.so резолвит лениво — если код реально не зовёт
# отсутствующий кодек, программа работает.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_link_options(-Wl,--unresolved-symbols=ignore-in-shared-libs)
endif()

# ============================================================
# RPATH для исполняемых файлов и shared libs
# ============================================================
# Бинарь app/VTL должен находить .so в external_libs/<pkg>/lib/ во время запуска.
# Используем $ORIGIN — относительный путь от бинаря — чтобы переместимый рабочий каталог
# не ломал сборку (app/ лежит в корне проекта, ../external_libs/.../lib относительно него).
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH
    "\$ORIGIN/../external_libs/ffmpeg/lib"
    "\$ORIGIN/../external_libs/curl/lib"
    "\$ORIGIN/../external_libs/postgresql/lib"
)

message(STATUS "VTL dependencies: external_libs/ (FFmpeg 60, curl 4, libpq 5)")
