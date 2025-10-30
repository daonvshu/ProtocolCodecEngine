cmake_minimum_required(VERSION 3.12)

project(ProtocolCodecEngine VERSION 0.0.9)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

if (WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
else ()
    set(CMAKE_CXX_FLAGS -Wno-attributes)
endif()

set(CMAKE_DEBUG_POSTFIX "d")

if (PROTOCOL_CODEC_ENGINE_BUILD_STATIC)
    set(_build_type STATIC)
else ()
    set(_build_type SHARED)
endif ()

add_library(${PROJECT_NAME} ${_build_type}
    protocolcodecengine.h
    protocolcodecengine.cpp
    protocolcodecinterface.h
    protocolcodecinterface.cpp

    utils/protocoltemplatedecoder.h
    utils/protocoltemplatedecoder.cpp
    utils/protocolflagdata.h
    utils/protocolflagdata.cpp

    flagdata/dataheader.h
    flagdata/dataheader.cpp
    flagdata/datasize.h
    flagdata/datasize.cpp
    flagdata/datacontent.h
    flagdata/datacontent.cpp
    flagdata/dataverify.h
    flagdata/dataverify.cpp
    flagdata/dataend.h
    flagdata/dataend.cpp
    flagdata/dataaddress.h
    flagdata/dataaddress.cpp
    flagdata/datatype.h
    flagdata/datatype.cpp

    verifier/protocolverify.h
    verifier/sumcheckverify.h
    verifier/sumcheckverify.cpp
    verifier/crccheckverify.h
    verifier/crccheckverify.cpp
    verifier/noneverify.h
    verifier/noneverify.cpp

    decoder/protocoldecoder.h
    decoder/protocoldecoder.cpp

    encoder/protocolencoder.h
    encoder/protocolencoder.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
)

if (PROTOCOL_CODEC_ENGINE_BUILD_STATIC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC PROTOCOL_CODEC_STATIC)
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE PROTOCOL_CODEC_LIBRARY)
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/..
    $<INSTALL_INTERFACE:include>
)

include(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
    EXPORT ProtocolCodecEngineTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/protocolcodec
    FILES_MATCHING PATTERN "*.h"
)

configure_package_config_file(${CMAKE_SOURCE_DIR}/cmake/ProtocolCodecEngineConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/ProtocolCodecEngineConfig.cmake"
    INSTALL_DESTINATION lib/cmake/ProtocolCodecEngine
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ProtocolCodecEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ProtocolCodecEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ProtocolCodecEngineConfigVersion.cmake"
    DESTINATION lib/cmake/ProtocolCodecEngine
)

install(EXPORT ProtocolCodecEngineTargets
    FILE ProtocolCodecEngineTargets.cmake
    NAMESPACE ProtocolCodec::
    DESTINATION lib/cmake/ProtocolCodecEngine
)