add_library(VTL_discord STATIC
        VTL_publication_text_op_discord.c
        VTL_publication_text_op_discord_convert.c
        ../VTL_publication_text_data.c
)

target_include_directories(VTL_discord PUBLIC ${CMAKE_SOURCE_DIR})

if (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(VTL_discord PUBLIC Threads::Threads)
endif ()

set_target_properties(VTL_discord PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)

if (MSVC)
    target_compile_options(VTL_discord PRIVATE /W4 /utf-8)
else ()
    target_compile_options(VTL_discord PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif ()
