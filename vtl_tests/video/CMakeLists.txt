set(VTL_CMAKE_VIDEO_TESTS_APP_NAME VTL_tests_Video)

set(VTL_CMAKE_VIDEO_TESTS_RUN VTL_tests_video_Run)

add_executable(${VTL_CMAKE_VIDEO_TESTS_APP_NAME} 
                ../VTL_test_data.c 
                VTL_frame_interpolation_unit.c)

target_include_directories(${VTL_CMAKE_VIDEO_TESTS_APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

# Добавляем зависимость OpenCL
find_package(OpenCL REQUIRED)
target_link_libraries(${VTL_CMAKE_VIDEO_TESTS_APP_NAME} PRIVATE OpenCL::OpenCL)

# Добавляем зависимости FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)
target_link_libraries(${VTL_CMAKE_VIDEO_TESTS_APP_NAME} PRIVATE PkgConfig::FFMPEG)

add_test(NAME ${VTL_CMAKE_VIDEO_TESTS_RUN} COMMAND ${VTL_CMAKE_VIDEO_TESTS_APP_NAME}) 