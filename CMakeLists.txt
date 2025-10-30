cmake_minimum_required(VERSION 3.12)

project(protocol_codec_engine_project)

option(PROTOCOL_CODEC_ENGINE_BUILD_STATIC "Build static libraries" ON)

add_subdirectory(src/protocolcodec)

option(PROTOCOL_CODEC_ENGINE_BUILD_TEST "enable protocol codec engine test project" OFF)

if (PROTOCOL_CODEC_ENGINE_BUILD_TEST)
    message("config protocol codec engine test!")
    add_subdirectory(test)
endif()