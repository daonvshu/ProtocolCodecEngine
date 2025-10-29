cmake_minimum_required(VERSION 3.12)

project(protocol_codec_engine_test)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

if (WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
else ()
    set(CMAKE_CXX_FLAGS -Wno-attributes)
endif()

add_executable(${PROJECT_NAME} WIN32
    main.cpp

    testutils.h
    testutils.cpp
    multitestrunner.h

    case/functionaltest.h
    case/functionaltest.cpp
    case/flagtest.h
    case/flagtest.cpp
    case/codectest.h
    case/codectest.cpp
    case/filetest.h
    case/filetest.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test

    protocol_codec_engine
)