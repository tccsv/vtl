add_library(VTL_scheduler OBJECT
    db/VTL_scheduler_repo.c
    metadata/VTL_scheduler_metadata.c
    dispatcher/VTL_scheduler_dispatcher.c
)

target_include_directories(VTL_scheduler PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(VTL_scheduler
    PUBLIC
        VTL_content_platform
        VTL_utils
        parson
        PostgreSQL::PostgreSQL
)

set_target_properties(VTL_scheduler PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_scheduler PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_scheduler PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
    )
endif()
