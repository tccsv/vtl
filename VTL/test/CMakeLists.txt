set(VTL_CMAKE_TESTS_APP_NAME VTL_Tests)

set(VTL_CMAKE_TESTS_RUN VTL_tests_Run)

add_executable(${VTL_CMAKE_TESTS_APP_NAME} VTL_test_data.c VTL_test.c)

target_include_directories(${VTL_CMAKE_TESTS_APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME ${VTL_CMAKE_TESTS_RUN} COMMAND ${VTL_CMAKE_TESTS_APP_NAME})

# Добавлено демо для прожига субтитров с поддержкой стилей и парсинга JSON
add_executable(VTL_sub_style_demo
    main_sub_style_demo.c
    ../media_container/sub/VTL_sub_convert.c
    ../media_container/sub/VTL_sub_style.c
    ../media_container/sub/VTL_sub_parse.c
    ../media_container/sub/VTL_sub_burn.c
    ../media_container/sub/infra/VTL_sub_read.c
    ../media_container/sub/infra/VTL_sub_write.c
    ../media_container/sub/VTL_sub_data.c
    ../media_container/sub/opencl/VTL_sub_opencl_strip_tags.c
    ../../external/parson/parson.c
)
target_include_directories(VTL_sub_style_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/VTL
    ${CMAKE_SOURCE_DIR}/VTL/media_container/sub
    ${CMAKE_SOURCE_DIR}/external/parson
    ${CMAKE_SOURCE_DIR}/external/ffmpeg
)
if(APPLE)
    target_link_libraries(VTL_sub_style_demo PRIVATE
        "-framework OpenCL"
        ${CMAKE_SOURCE_DIR}/external/ffmpeg/libavformat/libavformat.dylib
        ${CMAKE_SOURCE_DIR}/external/ffmpeg/libavcodec/libavcodec.dylib
        ${CMAKE_SOURCE_DIR}/external/ffmpeg/libavfilter/libavfilter.dylib
        ${CMAKE_SOURCE_DIR}/external/ffmpeg/libavutil/libavutil.dylib
        ${CMAKE_SOURCE_DIR}/external/ffmpeg/libswscale/libswscale.dylib
        ${CMAKE_SOURCE_DIR}/external/ffmpeg/libswresample/libswresample.dylib
    )
else()
    target_link_libraries(VTL_sub_style_demo PRIVATE OpenCL)
endif()
add_dependencies(VTL_sub_style_demo ffmpeg_ext)