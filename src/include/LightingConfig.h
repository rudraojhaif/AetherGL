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

struct AtmosphericConfig {
    bool enableAtmosphericFog = true;
    float fogDensity = 0.02f;
    float fogHeightFalloff = 0.1f;
    glm::vec3 fogColor = glm::vec3(0.7f, 0.8f, 0.9f);
    float atmosphericPerspective = 0.5f;
    
    // Full atmospheric scattering parameters
    float atmosphereRadius = 1000.0f;
    float planetRadius = 900.0f;
    float rayleighCoeff = 1.0f;
    float mieCoeff = 1.0f;
    float mieG = 0.8f;
    float sunDiskSize = 0.01f;
    float exposure = 1.0f;
};

struct TimeOfDayConfig {
    float timeOfDay = 0.3f;  // 0.0 = midnight, 0.5 = noon, 1.0 = midnight
    bool animateTimeOfDay = false;
    float daySpeed = 0.1f;   // Speed of day/night cycle
    
    // Automatic sun position calculation
    glm::vec3 getSunDirection() const {
        float angle = (timeOfDay - 0.5f) * PI * 2.0f; // -π to π
        return glm::normalize(glm::vec3(
            sin(angle) * 0.3f,  // Small horizontal movement
            -cos(angle),        // Main vertical movement
            -0.2f               // Slight forward bias
        ));
    }
    
    glm::vec3 getSunColor() const {
        // Color changes throughout day
        if (timeOfDay < 0.2f || timeOfDay > 0.8f) {
            // Night - very dim blue
            return glm::vec3(0.1f, 0.1f, 0.3f);
        } else if (timeOfDay < 0.3f || timeOfDay > 0.7f) {
            // Dawn/Dusk - warm colors
            return glm::vec3(1.0f, 0.6f, 0.3f);
        } else {
            // Day - bright white
            return glm::vec3(1.0f, 0.95f, 0.8f);
        }
    }
    
    float getSunIntensity() const {
        // Intensity varies throughout day
        if (timeOfDay < 0.2f || timeOfDay > 0.8f) {
            return 0.1f; // Night
        } else if (timeOfDay < 0.3f || timeOfDay > 0.7f) {
            return 2.0f; // Dawn/Dusk
        } else {
            return 4.0f; // Day
        }
    }
    
private:
    static constexpr float PI = 3.14159265359f;
};

class LightingConfig {
public:
    DirectionalLight directionalLight;
    std::vector<PointLight> pointLights;
    TerrainMaterialConfig terrain;
    POMConfig pom;
    IBLConfig ibl;
    AtmosphericConfig atmosphere;
    TimeOfDayConfig timeOfDay;
    
    LightingConfig() {
        setupDefaultLights();
    }
    
    /**
     * Apply all lighting settings to a shader
     */
    void applyToShader(std::shared_ptr<class Shader> shader, const glm::vec3& viewPos);
    
    /**
     * Update time-of-day animation
     */
    void updateTimeOfDay(float deltaTime);
    
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
