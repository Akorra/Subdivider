add_library(subdiv STATIC 
    src/subdiv.cpp
)

# Library headers
target_include_directories(subdiv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # subdiv headers
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm  # GLM headers
)

# Force latest C++ standard per target
target_compile_features(subdiv PUBLIC cxx_std_23)

# ---------------------
# Tests
# ---------------------
option(SUBDIV_BUILD_TESTS "Build subdiv tests" ON)

if(SUBDIV_BUILD_TESTS)
    add_subdirectory(tests)
endif()