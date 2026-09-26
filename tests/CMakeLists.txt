include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Catch2)

add_executable(iretable_tests
    test_text.cpp
    test_roundtrip.cpp
    test_cheattable.cpp
    test_diff.cpp
    test_rebase.cpp
    test_lint.cpp
    test_xml.cpp
)
target_include_directories(iretable_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(iretable_tests PRIVATE
    IRETABLE_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
target_link_libraries(iretable_tests PRIVATE iretable_core iretable_warnings Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(iretable_tests)
