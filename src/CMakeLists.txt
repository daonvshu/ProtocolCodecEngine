cmake_minimum_required(VERSION 3.12)

project(protocol_codec_engine)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(SRC_HEADERS
    ../include/protocolcodecengine.h

    ../include/utils/protocoltemplatedecoder.h
    ../include/utils/protocolflagdata.h
)

set(SRC_SOURCE
    protocolcodecengine.cpp

    utils/protocoltemplatedecoder.cpp
    utils/protocolflagdata.cpp
)

add_library(${PROJECT_NAME}
    ${SRC_HEADERS}
    ${SRC_SOURCE}
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
)