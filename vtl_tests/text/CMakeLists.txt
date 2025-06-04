set(VTL_CMAKE_TEXT_TESTS_APP_NAME VTL_tests_Text)
set(VTL_CMAKE_TEXT_TESTS_RUN VTL_tests_text_Run)
set(VTL_CMAKE_TEXT_IO_TESTS_APP_NAME VTL_tests_Text_IO)
set(VTL_CMAKE_TEXT_IO_TESTS_RUN VTL_tests_text_IO_Run)

add_executable(${VTL_CMAKE_TEXT_TESTS_APP_NAME}
        VTL_text_unit.c
    ../VTL_test_data.c
)

target_include_directories(${VTL_CMAKE_TEXT_TESTS_APP_NAME} 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${VTL_CMAKE_TEXT_TESTS_APP_NAME}
        PRIVATE
        libVTL
        ffmpeg
)


add_test(NAME ${VTL_CMAKE_TEXT_TESTS_RUN} COMMAND ${VTL_CMAKE_TEXT_TESTS_APP_NAME})

# Тест для файлового ввода-вывода
add_executable(${VTL_CMAKE_TEXT_IO_TESTS_APP_NAME}
        VTL_text_file_io_unit.c
    ../VTL_test_data.c
)

target_include_directories(${VTL_CMAKE_TEXT_IO_TESTS_APP_NAME} 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${VTL_CMAKE_TEXT_IO_TESTS_APP_NAME}
    PRIVATE
    libVTL
    ffmpeg
)

add_test(NAME ${VTL_CMAKE_TEXT_IO_TESTS_RUN} COMMAND ${VTL_CMAKE_TEXT_IO_TESTS_APP_NAME})

set_target_properties(${VTL_CMAKE_TEXT_TESTS_APP_NAME} ${VTL_CMAKE_TEXT_IO_TESTS_APP_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/app) 