cmake_minimum_required(VERSION 3.16)
project(Subdivider VERSION 0.1.0 LANGUAGES C CXX)

# Use the latest available C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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