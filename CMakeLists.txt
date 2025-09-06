cmake_minimum_required(VERSION 3.20)
project(ProceduralTerrain VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages (no Qt!)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
# find_package(imgui CONFIG REQUIRED)  # Temporarily disabled

# Create executable with GLFW main
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/include/Camera.cpp
        src/include/Shader.cpp
        src/include/Mesh.cpp
        src/include/TerrainGenerator.cpp
        src/include/NoiseGenerator.cpp
        src/include/LightingConfig.cpp
        src/include/HDRLoader.cpp
        src/include/PostProcessor.cpp
        src/include/TerrainRenderer.cpp
        src/include/ObjWriter.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        src/include
)

# Link libraries (pure OpenGL + GLFW + ImGui)
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glad::glad
        glm::glm
        opengl32
        # imgui::imgui  # Temporarily disabled
)

# Copy assets and shaders
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy shader files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/terrain_vertex.glsl ${CMAKE_CURRENT_BINARY_DIR}/terrain_vertex.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/pbr_terrain_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/pbr_terrain_fragment.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/skybox_vertex.glsl ${CMAKE_CURRENT_BINARY_DIR}/skybox_vertex.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/skybox_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/skybox_fragment.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/quad_vertex.glsl ${CMAKE_CURRENT_BINARY_DIR}/quad_vertex.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/bright_pass_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/bright_pass_fragment.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/blur_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/blur_fragment.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/final_postprocess_fragment.glsl ${CMAKE_CURRENT_BINARY_DIR}/final_postprocess_fragment.glsl COPYONLY)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
