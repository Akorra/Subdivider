add_library(subdiv STATIC 
    src/subdiv.cpp
    src/BaseMesh.cpp
)

# Library headers
target_include_directories(subdiv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # subdiv headers
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm  # GLM headers
)