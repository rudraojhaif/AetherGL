cmake_minimum_required(VERSION 3.20)
project(ProceduralTerrain VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Create executable
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/include/Camera.cpp
        src/include/Shader.cpp
        src/include/Mesh.cpp
        src/include/TerrainGenerator.cpp
        src/include/NoiseGenerator.cpp
        src/include/LightingConfig.cpp
        src/include/HDRLoader.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        src/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glad::glad
        glm::glm
)

# Copy shaders to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/terrain_vertex.glsl ${CMAKE_CURRENT_BINARY_DIR}/terrain_vertex.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/pbr_terrain_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/pbr_terrain_fragment.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/atmosphere_vertex.glsl ${CMAKE_CURRENT_BINARY_DIR}/atmosphere_vertex.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/atmosphere_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/atmosphere_fragment.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/skybox_vertex.glsl ${CMAKE_CURRENT_BINARY_DIR}/skybox_vertex.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/skybox_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/skybox_fragment.glsl COPYONLY)

# Set startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})