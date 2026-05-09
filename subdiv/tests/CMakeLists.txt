# Subdiv Library Tests using Catch2

# Fetch Catch2
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(Catch2)

include(CTest)
include(Catch)

# Helper function
function(add_subdiv_test TARGET)
    add_executable(${TARGET} ${ARGN})

    # Link libraries
    target_link_libraries(${TARGET} PRIVATE subdiv Catch2::Catch2WithMain )

    # Make sure tests compile with the same C++ standard
    target_compile_features(${TARGET} PRIVATE cxx_std_23)

    catch_discover_tests(${TARGET})
endfunction()

# Control tests
add_subdiv_test(control_mesh_tests      control/test_mesh.cpp)
add_subdiv_test(control_halfedge_tests  control/test_halfedge.cpp)
add_subdiv_test(control_meshcache_tests control/test_meshcache.cpp)

message(STATUS "Subdiv tests configured with Catch2")