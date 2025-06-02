set(VTL_CMAKE_AUDIO_TESTS_APP_NAME VTL_tests_Audio)

set(VTL_CMAKE_AUDIO_TESTS_RUN VTL_tests_audio_Run)

add_executable(${VTL_CMAKE_AUDIO_TESTS_APP_NAME} 
                ../VTL_test_data.c 
                VTL_audio_data_unit.c
                ${CMAKE_SOURCE_DIR}/VTL/media_container/audio/VTL_audio_data.c)

message(${CMAKE_SOURCE_DIR})

target_include_directories(${VTL_CMAKE_AUDIO_TESTS_APP_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/build/ffmpeg_install/include
)

if(APPLE)
    target_link_libraries(${VTL_CMAKE_AUDIO_TESTS_APP_NAME} PRIVATE ffmpeg z iconv)
elseif(UNIX)
    target_link_libraries(${VTL_CMAKE_AUDIO_TESTS_APP_NAME} PRIVATE ffmpeg z bz2 lzma zstd m pthread iconv)
endif()

add_test(NAME ${VTL_CMAKE_AUDIO_TESTS_RUN} COMMAND ${VTL_CMAKE_AUDIO_TESTS_APP_NAME})
