add_library(VTL_user OBJECT
    VTL_user_data.c
    VTL_error_data.c
    history/VTL_user_history_data.c
    history/db/VTL_user_history_save.c
)

target_include_directories(VTL_user PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VTL_user PUBLIC VTL_utils)

set_target_properties(VTL_user PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_user PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_user PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
    )
endif()
