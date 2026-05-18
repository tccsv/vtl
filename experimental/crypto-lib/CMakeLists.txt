cmake_minimum_required(VERSION 3.10)
project(crypto-lib C)

set(CMAKE_C_STANDARD 17)

include_directories(include)
include_directories(external/ed25519)

file(GLOB ED25519_SOURCES "external/ed25519/*.c")

set(AUTH_SOURCES
    src/VTL_auth.c
    src/VTL_keys.c
)

add_library(vtl_auth STATIC
    ${AUTH_SOURCES}
    ${ED25519_SOURCES}
)
