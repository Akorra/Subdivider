include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(subdiv_tests
    test_utils.cpp
    test_mesh.cpp
)

target_link_libraries(subdiv_tests
    PRIVATE
        subdiv
        Catch2::Catch2WithMain
)

# Make sure tests compile with the same C++ standard
target_compile_features(subdiv_tests PRIVATE cxx_std_23)

include(CTest)
include(Catch)
catch_discover_tests(subdiv_tests)