cmake_minimum_required(VERSION 3.20)

if(NOT PROJECT_NAME)
    # export EvnVar VCPKG_ROOT 
    # set vcpkg triplet and toolchain_file, Must before project()
    if(WIN32)
        if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
            set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        endif()
        if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
        elseif(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x86-windows" CACHE STRING "")
        endif()
        message("CMAKE_TOOLCHAIN_FILE: " ${CMAKE_TOOLCHAIN_FILE})
        message("VCPKG_TARGET_TRIPLET: " ${VCPKG_TARGET_TRIPLET})
    endif()

    project(Core LANGUAGES CXX C)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY        ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY        ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY        ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    # set windows export symbols(spawn .lib)
    add_compile_options(/MP)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS_SAVE ${CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS})
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
endif()

file(GLOB_RECURSE CoreHeader Include/*.h)
file(GLOB_RECURSE CoreSource Source/*.cpp)
file(GLOB_RECURSE CoreReflect Reflect/*.cpp)

add_library(Core SHARED
    ${CoreHeader}
    ${CoreSource}
    ${CoreReflect})

if(NOT TARGET asio)
    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/asio/asio/include>
        $<INSTALL_INTERFACE:Include>
        )
    target_compile_definitions(asio
        INTERFACE ASIO_STANDALONE
        )
endif()

target_link_libraries(Core PRIVATE asio)

target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>    #${CMAKE_INSTALL_PREFIX}/include
    )

# For windows export symbols
include(GenerateExportHeader)
generate_export_header(Core EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Include/CoreExport.h)


target_compile_definitions(Core PUBLIC CORE_MODULE)

if("${PROJECT_NAME}" STREQUAL "Core")
    add_subdirectory(Tests)
endif()

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ${CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS_SAVE})
endif()

target_include_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Reflect)

foreach(SourceFile ${CoreSource})
    set(SourceFileInput "${SourceFileInput} ${SourceFile}")
endforeach()

set(ReflectorExecutor Reflector.exe)
set(ReflectorExecuteCommand "${ReflectorExecutor} -p ${CMAKE_CURRENT_BINARY_DIR} ${SourceFileInput}")

file(GENERATE OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/reflector.bat"
    CONTENT "cd ${CMAKE_CURRENT_SOURCE_DIR}/Reflect && ${ReflectorExecuteCommand}")
  