#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

/**
 * LightingConfig - Easy-to-use lighting configuration for PBR rendering
 * 
 * This structure makes it simple to configure and modify lighting parameters
 * from main.cpp without dealing with individual uniform calls.
 */

struct DirectionalLight {
    glm::vec3 direction = glm::vec3(0.3f, -0.7f, -0.2f);
    glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.8f);
    float intensity = 3.0f;
    bool enabled = true;
    
    DirectionalLight() = default;
    DirectionalLight(const glm::vec3& dir, const glm::vec3& col, float intens)
        : direction(dir), color(col), intensity(intens) {}
};

struct PointLight {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    float range = 20.0f;
    
    PointLight() = default;
    PointLight(const glm::vec3& pos, const glm::vec3& col, float intens, float r)
        : position(pos), color(col), intensity(intens), range(r) {}
};

struct TerrainMaterialConfig {
    float grassHeight = 2.0f;
    float rockHeight = 8.0f;
    float snowHeight = 12.0f;
};

struct POMConfig {
    bool enabled = true;
    float scale = 0.08f;
    int minSamples = 16;
    int maxSamples = 64;
    float sharpen = 1.0f;
};

struct IBLConfig {
    bool enabled = false;
    float intensity = 0.3f;
};

class LightingConfig {
public:
    DirectionalLight directionalLight;
    std::vector<PointLight> pointLights;
    TerrainMaterialConfig terrain;
    POMConfig pom;
    IBLConfig ibl;
    
    LightingConfig() {
        setupDefaultLights();
    }
    
    /**
     * Apply all lighting settings to a shader
     */
    void applyToShader(std::shared_ptr<class Shader> shader, const glm::vec3& viewPos);
    
    /**
     * Setup default lighting scenario
     */
    void setupDefaultLights();
    
    /**
     * Setup dramatic sunset lighting
     */
    void setupSunsetLighting();
    
    /**
     * Setup night scene with multiple point lights
     */
    void setupNightLighting();
    
    /**
     * Add a point light to the scene
     */
    void addPointLight(const PointLight& light);
    
    /**
     * Remove all point lights
     */
    void clearPointLights();
};
