# 3.28 for EXCLUDE_FROM_ALL in FetchContent_Declare, matching Pointer Lab and
# Signature Lab. iretable-tools has no fetched runtime dependency, but the tests
# fetch Catch2, and keeping the floor the same avoids surprises across the set.
cmake_minimum_required(VERSION 3.28)

project(iretable-tools VERSION 1.0.1 LANGUAGES CXX)

if(NOT MSVC)
    # The tool is standard C++20 and would build elsewhere, but it is developed,
    # tested and released on MSVC only, like the rest of the Game Reversal Club
    # tools. Configuring with another compiler is unsupported rather than
    # forbidden; flip this to a warning if you want to try it.
    message(FATAL_ERROR "iretable-tools is built and tested with MSVC only.")
endif()

get_property(_ire_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _ire_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Static CRT, so the released iretable.exe runs on a clean Windows machine with
# no Visual C++ redistributable installed.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(IRETABLE_BUILD_TESTS "Build the unit tests" ON)
option(IRETABLE_WARNINGS_AS_ERRORS "Treat first-party compiler warnings as errors" ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IRETABLE_ARCH x64)
else()
    set(IRETABLE_ARCH x86)
endif()

# ---------------------------------------------------------------------------
# Warnings, first-party targets only
# ---------------------------------------------------------------------------

add_library(iretable_warnings INTERFACE)
target_compile_options(iretable_warnings INTERFACE /W4 /permissive- /Zc:__cplusplus /utf-8)
if(IRETABLE_WARNINGS_AS_ERRORS)
    target_compile_options(iretable_warnings INTERFACE /WX)
endif()

# ---------------------------------------------------------------------------
# iretable_core: the library. No Windows header, no runtime dependency, so the
# tests link the real logic and it would port to another platform unchanged.
# ---------------------------------------------------------------------------

add_library(iretable_core STATIC
    src/iretable/CheatTable.cpp
    src/iretable/Diff.cpp
    src/iretable/Json.cpp
    src/iretable/Lint.cpp
    src/iretable/Reader.cpp
    src/iretable/Rebase.cpp
    src/iretable/Show.cpp
    src/iretable/Text.cpp
    src/iretable/ValueType.cpp
    src/iretable/Writer.cpp
    src/iretable/Xml.cpp
)
target_compile_features(iretable_core PUBLIC cxx_std_20)
target_include_directories(iretable_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(iretable_core PRIVATE iretable_warnings)

# ---------------------------------------------------------------------------
# The CLI.
# ---------------------------------------------------------------------------

add_executable(iretable src/cli/main.cpp)
target_link_libraries(iretable PRIVATE iretable_core iretable_warnings)
set_target_properties(iretable PROPERTIES OUTPUT_NAME iretable)
# --version prints the project() version, so it always matches the zip name.
target_compile_definitions(iretable PRIVATE IRETABLE_VERSION="${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(IRETABLE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Package: a zip with the CLI and the documents.
# ---------------------------------------------------------------------------

install(TARGETS iretable RUNTIME DESTINATION .)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md
    ${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    DESTINATION .
)

set(CPACK_PACKAGE_NAME "iretable-tools")
set(CPACK_PACKAGE_VENDOR "Game Reversal Club")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "iretable-tools-v${PROJECT_VERSION}-windows-${IRETABLE_ARCH}")
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Command-line toolkit for Pointer Lab .iretable project files")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
include(CPack)
