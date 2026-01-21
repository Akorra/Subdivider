cmake_minimum_required(VERSION 3.10.0)
project(Subdivider VERSION 0.1.0 LANGUAGES C CXX)

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