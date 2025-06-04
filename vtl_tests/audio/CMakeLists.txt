set(VTL_CMAKE_AUDIO_TESTS_APP_NAME VTL_tests_Audio)

set(VTL_CMAKE_AUDIO_TESTS_RUN VTL_tests_audio_Run)

add_executable(${VTL_CMAKE_AUDIO_TESTS_APP_NAME} 
                ../VTL_test_data.c 
                VTL_audio_data_unit.c)

message(${CMAKE_SOURCE_DIR})

target_include_directories(${VTL_CMAKE_AUDIO_TESTS_APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME ${VTL_CMAKE_AUDIO_TESTS_RUN} COMMAND ${VTL_CMAKE_AUDIO_TESTS_APP_NAME})
