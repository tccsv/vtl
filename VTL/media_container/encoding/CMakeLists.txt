cmake_minimum_required(VERSION 3.25)
project(libvencode
    LANGUAGES C
    VERSION 1.0.0
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)
include(ExternalProject)
include(GNUInstallDirs)

FetchContent_Declare(
    vulkan-headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG        v1.3.261 
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(
    vulkan-loader
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG        v1.3.261
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(vulkan-headers)

set(VULKAN_HEADERS_INSTALL_DIR "${vulkan-headers_SOURCE_DIR}" CACHE PATH "" FORCE)
set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(vulkan-loader)

# --- NASM ---
find_program(NASM_EXE nasm)
if(NOT NASM_EXE)
    message(FATAL_ERROR "nasm not found – install nasm >= 2.14")
endif()

# --- FFmpeg & Dependencies ---
set(FFMPEG_INSTALL "${CMAKE_BINARY_DIR}/ffmpeg-install")
set(FFMPEG_INCLUDE "${FFMPEG_INSTALL}/include")
set(FFMPEG_LIB_DIR "${FFMPEG_INSTALL}/lib")

file(MAKE_DIRECTORY ${FFMPEG_INCLUDE})
file(MAKE_DIRECTORY ${FFMPEG_LIB_DIR})

set(FFMPEG_EXTRA_CONFIG "")
if(WIN32)
    list(APPEND FFMPEG_EXTRA_CONFIG "--arch=x86_64" "--target-os=mingw32")
elseif(APPLE)
    list(APPEND FFMPEG_EXTRA_CONFIG "--arch=arm64" "--target-os=darwin")
endif()

function(_add_ffmpeg_lib name)
    add_library(ffmpeg::${name} STATIC IMPORTED GLOBAL)
    set_target_properties(ffmpeg::${name} PROPERTIES
        IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${name}${CMAKE_STATIC_LIBRARY_SUFFIX}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE}"
    )
    if(UNIX AND NOT APPLE)
        target_link_libraries(ffmpeg::${name} INTERFACE m pthread dl z bz2 lzma x264 x265 X11 va va-drm va-x11 vdpau)
    elseif(APPLE)
        target_link_libraries(ffmpeg::${name} INTERFACE "-framework CoreFoundation" "-framework CoreVideo" "-framework CoreMedia" z bz2 lzma)
    elseif(WIN32)
        target_link_libraries(ffmpeg::${name} INTERFACE bcrypt strmiids uuid ole32 user32)
    endif()
    add_dependencies(ffmpeg::${name} ffmpeg_external)
endfunction()

ExternalProject_Add(
    ffmpeg_external
    GIT_REPOSITORY   https://github.com/FFmpeg/FFmpeg.git
    GIT_TAG          n6.0
    GIT_SHALLOW      TRUE
    PATCH_COMMAND    patch -p1 < ${CMAKE_SOURCE_DIR}/ffmpeg_nasm_fix.patch
    UPDATE_DISCONNECTED TRUE
    BUILD_IN_SOURCE  FALSE
    CONFIGURE_COMMAND
        <SOURCE_DIR>/configure
        --prefix=${FFMPEG_INSTALL}
        --enable-static --disable-shared
        --disable-programs --disable-doc
        --disable-zlib --disable-lzma --disable-iconv --disable-libxcb
        --enable-avdevice --enable-avfilter --enable-avformat
        --enable-avcodec --enable-swresample --enable-swscale --enable-avutil
        --enable-gpl
        --enable-libx264
        --enable-libx265
        --enable-vulkan
        --extra-cflags=-I$<TARGET_PROPERTY:Vulkan::Headers,INTERFACE_INCLUDE_DIRECTORIES>
        --extra-ldflags=-L$<TARGET_LINKER_FILE_DIR:Vulkan::Vulkan>
        ${FFMPEG_EXTRA_CONFIG}
    BUILD_COMMAND     make -j${CMAKE_HOST_PROCESSOR_COUNT}
    INSTALL_COMMAND   make install
)

foreach(lib avutil swresample swscale avcodec avformat avfilter avdevice)
    _add_ffmpeg_lib(${lib})
endforeach()

# --- VTL Module Definition ---
set(VTL_VENCODE_SRC_FILES
    VTL_vencode_core.c
    VTL_vencode_loader.c
    VTL_vencode_transcoder.c
    VTL_vencode_cpu.c
    VTL_vencode_vk.c
)

add_library(vencode STATIC ${VTL_VENCODE_SRC_FILES})

target_include_directories(vencode
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${FFMPEG_INCLUDE}
        ${vulkan-headers_SOURCE_DIR}/include
)

target_link_libraries(vencode
    PRIVATE
        ffmpeg::avdevice ffmpeg::avfilter ffmpeg::avformat
        ffmpeg::avcodec  ffmpeg::swresample
        ffmpeg::swscale  ffmpeg::avutil
        Vulkan::Vulkan
)

# --- Tests & Examples ---
add_executable(demo_all_modes examples/demo_all_modes.c)
target_include_directories(demo_all_modes PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> ${FFMPEG_INCLUDE})
target_link_libraries(demo_all_modes PRIVATE vencode)

install(TARGETS vencode
        EXPORT  vencodeTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
