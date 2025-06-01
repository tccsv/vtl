set(VTL_CMAKE_PUBLICATION_TESTS_APP_NAME VTL_tests_Publication)

set(VTL_CMAKE_PUBLICATION_TESTS_RUN VTL_tests_publication_Run)

add_executable(${VTL_CMAKE_PUBLICATION_TESTS_APP_NAME} 
                ../VTL_test_data.c 
                VTL_publication_unit.c)

target_include_directories(${VTL_CMAKE_PUBLICATION_TESTS_APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME ${VTL_CMAKE_PUBLICATION_TESTS_RUN} COMMAND ${VTL_CMAKE_PUBLICATION_TESTS_APP_NAME}) 