cmake_minimum_required(VERSION 3.12)

project(protocol_codec_engine_project)

add_subdirectory(src)

option(PROTOCOL_CODEC_ENGINE_BUILD_TEST "enable protocol codec engine test project" OFF)

if (WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
else ()
    set(CMAKE_CXX_FLAGS -Wno-attributes)
endif()

if (PROTOCOL_CODEC_ENGINE_BUILD_TEST)
    message("config protocol codec engine test!")
    add_subdirectory(test)
endif()