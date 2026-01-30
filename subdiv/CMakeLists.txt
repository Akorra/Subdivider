add_library(subdiv STATIC 
    src/subdiv.cpp
    src/control/mesh_cache.cpp
    src/control/mesh.cpp
    src/diagnostics/context.cpp
)

# Library headers
target_include_directories(subdiv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # subdiv headers
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm  # GLM headers
)

# Force latest C++ standard per target
target_compile_features(subdiv PUBLIC cxx_std_23)

# ---------------------
# Diagnostic Configuration
# ---------------------

# Pass build configuration as compile definitions
target_compile_definitions(subdiv
    PUBLIC
        $<$<BOOL:${SUBDIV_ENABLE_PROFILING}>:SUBDIV_ENABLE_PROFILING>
        $<$<BOOL:${SUBDIV_ENABLE_MEMORY_TRACKING}>:SUBDIV_ENABLE_MEMORY_TRACKING>
        $<$<BOOL:${SUBDIV_ENABLE_VALIDATION}>:SUBDIV_ENABLE_VALIDATION>
    PRIVATE
        $<$<BOOL:${SUBDIV_ENABLE_ASSERTS}>:SUBDIV_ENABLE_ASSERTS>
)

# Compiler warnings
if(MSVC)
    target_compile_options(subdiv PRIVATE 
        /W4
        # /WX  # Treat warnings as errors (optional)
    )
else()
    target_compile_options(subdiv PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        # -Werror  # Treat warnings as errors (optional)
    )
endif()

# Print configuration summary
message(STATUS "----------------------------------------------")
message(STATUS "Subdiv Library Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Profiling: ${SUBDIV_ENABLE_PROFILING}")
message(STATUS "  Memory Tracking: ${SUBDIV_ENABLE_MEMORY_TRACKING}")
message(STATUS "  Validation: ${SUBDIV_ENABLE_VALIDATION}")
message(STATUS "  Asserts: ${SUBDIV_ENABLE_ASSERTS}")
# Print library configuration
message(STATUS "----------------------------------------------")
message(STATUS "Subdiv Library:")
message(STATUS "  Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Binary Dir: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "----------------------------------------------")

# ---------------------
# Tests
# ---------------------
# Define the option if not already defined
if(NOT DEFINED SUBDIV_BUILD_TESTS)
    set(SUBDIV_BUILD_TESTS ON CACHE BOOL "Build subdiv tests")
endif()

if(SUBDIV_BUILD_TESTS)
    message(STATUS "Subdiv tests: ENABLED")
    add_subdirectory(tests)
else()
    message(STATUS "Subdiv tests: DISABLED")
endif()