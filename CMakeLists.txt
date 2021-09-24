
cmake_minimum_required(VERSION 3.17)
if(NOT PROJECT_NAME)
    project(Core LANGUAGES CXX C)
endif()

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY        ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY        ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY        ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

set (CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(WIN32)
    # set windows export symbols(spawn .lib)
    add_compile_options(/MP)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

file(GLOB_RECURSE CoreHeader Include/*.h)
file(GLOB_RECURSE CoreSource Source/*.cpp)

add_library(Core SHARED
    ${CoreHeader}
    ${CoreSource})

target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>    #${CMAKE_INSTALL_PREFIX}/include
    )

# For windows export symbols
include(GenerateExportHeader)
generate_export_header(Core EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/Include/CoreExport.h)
target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Include> #for export header
    )

if("${PROJECT_NAME}" STREQUAL "Core")
    add_subdirectory(Tests)
endif()