add_library(VTL_publication OBJECT
    VTL_publication.c
    text/VTL_publication_text_data.c
    text/VTL_publication_text_op.c
    text/VTL_publication_markup_text_flags_check.c
    text/infra/VTL_publication_text_read.c
    text/infra/VTL_publication_text_write.c
    text/bbcode/VTL_publication_text_op_bbcode.c
    text/asciidoc/VTL_publication_text_op_asciidoc.c
    text/asciidoc/VTL_publication_text_op_asciidoc_data.c
    text/asciidoc/VTL_publication_text_op_asciidoc_scan.c
    text/asciidoc/VTL_publication_text_op_asciidoc_convert.c
    text/asciidoc/infra/VTL_publication_text_op_asciidoc_load.c
    text/mediawiki/VTL_publication_text_op_mediawiki.c
    text/mediawiki/VTL_publication_text_op_mediawiki_data.c
    text/mediawiki/VTL_publication_text_op_mediawiki_scan.c
    text/mediawiki/VTL_publication_text_op_mediawiki_convert.c
)

if(UNIX)
    target_link_libraries(VTL_publication PUBLIC pthread)
endif()

target_include_directories(VTL_publication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VTL_publication PUBLIC VTL_utils)

set_target_properties(VTL_publication PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_publication PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_publication PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
    )
endif()
