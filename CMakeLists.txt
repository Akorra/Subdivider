cmake_minimum_required(VERSION 3.16)
project(Subdivider VERSION 0.1.0 LANGUAGES C CXX)

# Use the latest available C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type options
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "Profile")

# Custom Profile build type
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2" CACHE STRING "Flags for Profile build")
set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O2" CACHE STRING "Flags for Profile build")
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "Linker flags for Profile build")

# Options
option(SUBDIV_ENABLE_PROFILING "Enable profiling instrumentation" OFF)
option(SUBDIV_ENABLE_MEMORY_TRACKING "Enable memory tracking" OFF)
option(SUBDIV_ENABLE_VALIDATION "Enable runtime validation checks" ON)
option(SUBDIV_ENABLE_ASSERTS "Enable assertions" ON)

# Auto-enable profiling for Profile build type
if(CMAKE_BUILD_TYPE STREQUAL "Profile")
    set(SUBDIV_ENABLE_PROFILING ON CACHE BOOL "Enable profiling instrumentation" FORCE)
    set(SUBDIV_ENABLE_MEMORY_TRACKING ON CACHE BOOL "Enable memory tracking" FORCE)
    set(SUBDIV_ENABLE_VALIDATION ON CACHE BOOL "Enable runtime validation checks" FORCE)
endif()

# Auto-disable features for Release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(SUBDIV_ENABLE_PROFILING OFF CACHE BOOL "Enable profiling instrumentation" FORCE)
    set(SUBDIV_ENABLE_MEMORY_TRACKING OFF CACHE BOOL "Enable memory tracking" FORCE)
    set(SUBDIV_ENABLE_ASSERTS OFF CACHE BOOL "Enable assertions" FORCE)
endif()

# Print project configuration
message(STATUS "==============================================")
message(STATUS "Subdivider Project Configuration")
message(STATUS "==============================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "----------------------------------------------")
message(STATUS "Diagnostic Features:")
message(STATUS "  Profiling:        ${SUBDIV_ENABLE_PROFILING}")
message(STATUS "  Memory Tracking:  ${SUBDIV_ENABLE_MEMORY_TRACKING}")
message(STATUS "  Validation:       ${SUBDIV_ENABLE_VALIDATION}")
message(STATUS "  Asserts:          ${SUBDIV_ENABLE_ASSERTS}")
message(STATUS "==============================================")

enable_testing()

# Build the library first
add_subdirectory(subdiv)

# Build the consumer app
add_subdirectory(consumerApp)

# Optional: add a custom target to run the consumer app
add_custom_target(runConsumer
    COMMAND consumerApp
    DEPENDS consumerApp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/consumerApp
    COMMENT "Running consumer application..."
)