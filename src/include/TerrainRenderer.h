#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "LightingConfig.h"
#include "HDRLoader.h"
#include "PostProcessor.h"

class TerrainRenderer {
public:
    TerrainRenderer();
    ~TerrainRenderer();

    // Initialization
    void initialize(int width, int height);
    void resize(int width, int height);

    // Rendering
    void render();

    // Input handling
    void processMouseMovement(float xoffset, float yoffset);
    void processMouseScroll(float yoffset);
    void processKeyboard(int key, int action);

    // Post-processing controls
    void toggleBloom();
    void toggleDOF();
    void toggleChromaticAberration();
    
    // GUI access methods
    glm::vec3 getCameraPosition() const;
    LightingConfig& getLightingConfig();
    bool isPostProcessingEnabled() const;
    void setPostProcessingEnabled(bool enabled);
    PostProcessor::Config getPostProcessorConfig() const;
    void setPostProcessorConfig(const PostProcessor::Config& config);

private:
    // Setup methods
    void setupShaders();
    void setupTerrain();
    void setupLighting();
    void setupPostProcessing();

    // Rendering methods
    void renderScene();
    void updateDeltaTime();

    // Core components
    std::unique_ptr<Camera> m_camera;
    std::shared_ptr<Shader> m_terrainShader;
    std::shared_ptr<Shader> m_skyboxShader;
    std::shared_ptr<Mesh> m_terrainMesh;
    std::unique_ptr<PostProcessor> m_postProcessor;

    // Lighting and environment
    LightingConfig m_lighting;
    HDRLoader::IBLTextures m_iblTextures;

    // Viewport and timing
    int m_width = 1600;
    int m_height = 1200;
    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;

    // Input state
    bool m_keysPressed[1024] = {false};
    
    // Post-processing state
    bool m_postProcessingEnabled = true;
    
    // Debug info
    void printPostProcessingStatus() const;
};
