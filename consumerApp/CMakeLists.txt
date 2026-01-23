add_executable(consumerApp 
    main.cpp 
    src/app.cpp
)

# Library headers
target_include_directories(consumerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(consumerApp PRIVATE subdiv)

# Find OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(consumerApp PRIVATE OpenGL::GL)

# GLFW (modern way is to use FetchContent)
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)
FetchContent_MakeAvailable(glfw)
target_link_libraries(consumerApp PRIVATE glfw)

# GLAD
FetchContent_Declare(
    glad2
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8 
)
FetchContent_MakeAvailable(glad2)

# Add the CMake folder from glad2 as a subdirectory
# GLAD2 includes its own CMake config for creating targets
add_subdirectory(
    ${glad2_SOURCE_DIR}/cmake
    ${glad2_BINARY_DIR}/cmake
)

# Create a GLAD loader target for OpenGL core 3.3
glad_add_library(
    glad_gl_core_33
    REPRODUCIBLE
    API gl:core=3.3
)

# Link the created glad loader with our executable
target_link_libraries(consumerApp PRIVATE glad_gl_core_33)