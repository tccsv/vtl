set(VTL_CMAKE_TESTS_APP_NAME VTL_Tests)

set(VTL_CMAKE_TESTS_RUN VTL_tests_Run)

add_executable(${VTL_CMAKE_TESTS_APP_NAME} VTL_test_data.c VTL_test.c)

target_include_directories(${VTL_CMAKE_TESTS_APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME ${VTL_CMAKE_TESTS_RUN} COMMAND ${VTL_CMAKE_TESTS_APP_NAME})


find_package(Threads REQUIRED)

set(VTL_CMAKE_TESTS_MW_APP_NAME VTL_Tests_MediaWiki)
add_executable(${VTL_CMAKE_TESTS_MW_APP_NAME}
    VTL_test_data.c
    VTL_test_mediawiki.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/VTL_publication_text_data.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_data.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_scan.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_convert.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/telegram/VTL_publication_text_op_telegram.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/telegram/VTL_publication_text_op_telegram_data.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/telegram/VTL_publication_text_op_telegram_scan.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/telegram/VTL_publication_text_op_telegram_convert.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/markdown/VTL_publication_text_op_markdown.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/markdown/VTL_publication_text_op_markdown_data.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/markdown/VTL_publication_text_op_markdown_scan.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/markdown/VTL_publication_text_op_markdown_convert.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/html/VTL_publication_text_op_html.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/html/VTL_publication_text_op_html_data.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/html/VTL_publication_text_op_html_scan.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/html/VTL_publication_text_op_html_convert.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_data.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_scan.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_convert.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/VTL_publication_markup_text_flags_check.c
    ${CMAKE_SOURCE_DIR}/VTL/publication/text/VTL_publication_text_op.c
)
target_include_directories(${VTL_CMAKE_TESTS_MW_APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${VTL_CMAKE_TESTS_MW_APP_NAME} PRIVATE Threads::Threads)
target_compile_definitions(${VTL_CMAKE_TESTS_MW_APP_NAME} PRIVATE
    VTL_WIKI_FIXTURE="${CMAKE_SOURCE_DIR}/text.wiki"
)
set_target_properties(${VTL_CMAKE_TESTS_MW_APP_NAME} PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON
)
add_test(NAME VTL_tests_MediaWiki_Run COMMAND ${VTL_CMAKE_TESTS_MW_APP_NAME})