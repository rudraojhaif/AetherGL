#pragma once

#include <GLFW/glfw3.h>
#include <memory>

class TerrainRenderer;

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    // Initialize ImGui
    bool initialize(GLFWwindow* window);
    void shutdown();

    // Render GUI
    void render(TerrainRenderer* renderer);
    
    // Frame management
    void beginFrame();
    void endFrame();

private:
    bool m_initialized = false;
    
    // GUI state
    bool m_showDemo = false;
    bool m_showSettings = true;
    
    // Render all GUI panels
    void renderMainSettings(TerrainRenderer* renderer);
    void renderLightingSettings(TerrainRenderer* renderer);
    void renderPostProcessingSettings(TerrainRenderer* renderer);
    void renderTerrainSettings(TerrainRenderer* renderer);
    
    // Helper functions
    void helpMarker(const char* desc);
};
