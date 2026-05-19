# macOS-сборка. Все .dylib из Homebrew bottles (arm64_sonoma) лежат в
# external_libs/macos/. Никаких brew install — всё в репо.

set(MAC_DIR "${EXTERNAL_LIBS_DIR}/macos")
set(MAC_LIB "${MAC_DIR}/lib")
set(MAC_INC "${MAC_DIR}/include")

# ============================================================
# FFmpeg
# ============================================================
add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg INTERFACE "${MAC_INC}")

macro(_vtl_mac_ffmpeg comp ver)
    add_library(ffmpeg_${comp} SHARED IMPORTED)
    set_target_properties(ffmpeg_${comp} PROPERTIES
        IMPORTED_LOCATION "${MAC_LIB}/lib${comp}.${ver}.dylib"
    )
    target_link_libraries(ffmpeg INTERFACE ffmpeg_${comp})
endmacro()

_vtl_mac_ffmpeg(avcodec    62)
_vtl_mac_ffmpeg(avformat   62)
_vtl_mac_ffmpeg(avutil     60)
_vtl_mac_ffmpeg(avfilter   11)
_vtl_mac_ffmpeg(swscale     9)
_vtl_mac_ffmpeg(swresample  6)

# ============================================================
# libcurl
# ============================================================
add_library(CURL::libcurl SHARED IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LOCATION "${MAC_LIB}/libcurl.4.dylib"
    INTERFACE_INCLUDE_DIRECTORIES "${MAC_INC}"
)

# ============================================================
# libpq (PostgreSQL)
# ============================================================
add_library(PostgreSQL::PostgreSQL SHARED IMPORTED)
set_target_properties(PostgreSQL::PostgreSQL PROPERTIES
    IMPORTED_LOCATION "${MAC_LIB}/libpq.5.dylib"
    INTERFACE_INCLUDE_DIRECTORIES "${MAC_INC}"
)

# ============================================================
# RPATH для бинаря и патчинг install_name на каждом dylib.
# Brew bottles имеют install_name = /opt/homebrew/Cellar/.../*.dylib
# (жёсткий абсолютный путь). На пользовательском Mac без brew он не работает.
# При configure запускаем install_name_tool чтобы каждый dylib имел
# id = @rpath/<basename>, а ссылки внутри — на @rpath/<dep>.
# Это безопасная idempotent-операция (повторный запуск пропускает уже @rpath).
# ============================================================
find_program(_VTL_INSTALL_NAME_TOOL install_name_tool REQUIRED)
find_program(_VTL_OTOOL otool REQUIRED)
find_program(_VTL_CODESIGN codesign REQUIRED)

file(GLOB _VTL_MAC_DYLIBS "${MAC_LIB}/*.dylib")
foreach(dylib ${_VTL_MAC_DYLIBS})
    get_filename_component(name "${dylib}" NAME)

    # 1. Меняем id: @rpath/<basename>
    execute_process(
        COMMAND "${_VTL_INSTALL_NAME_TOOL}" -id "@rpath/${name}" "${dylib}"
        OUTPUT_QUIET ERROR_QUIET
    )

    # 2. Для каждой ссылки на /opt/homebrew/... меняем на @rpath/<basename>
    execute_process(
        COMMAND "${_VTL_OTOOL}" -L "${dylib}"
        OUTPUT_VARIABLE _otool_out
        ERROR_QUIET
    )
    string(REGEX MATCHALL "/opt/homebrew/[^ \n]+\\.dylib" _deps "${_otool_out}")
    foreach(dep ${_deps})
        get_filename_component(depname "${dep}" NAME)
        execute_process(
            COMMAND "${_VTL_INSTALL_NAME_TOOL}" -change "${dep}" "@rpath/${depname}" "${dylib}"
            OUTPUT_QUIET ERROR_QUIET
        )
    endforeach()

    # 3. После install_name_tool подпись dylib становится invalid — на
    #    Apple Silicon это приводит к 'zsh: killed' при загрузке. Re-sign ad-hoc.
    execute_process(
        COMMAND "${_VTL_CODESIGN}" --force --sign - "${dylib}"
        OUTPUT_QUIET ERROR_QUIET
    )
endforeach()

# RPATH бинаря: ищем .dylib через @executable_path относительно app/
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "@executable_path/../external_libs/macos/lib")

message(STATUS "VTL macOS: ${MAC_DIR} (FFmpeg 8, curl 4, libpq 5)")
