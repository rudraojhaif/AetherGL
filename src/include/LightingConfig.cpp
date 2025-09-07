// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is the property of Rudra Ojha.  
// Redistribution, modification, or use of this code in any project  
// (commercial or non-commercial) is strictly prohibited without  
// prior written consent from the author.
//
// Unauthorized usage will be considered a violation of copyright law.

#include "LightingConfig.h"
#include "Shader.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <cmath>

void LightingConfig::applyToShader(std::shared_ptr<Shader> shader, const glm::vec3& viewPos) {
    shader->use();
    
    // View position
    shader->setVec3("viewPos", viewPos);
    
    // Directional Light
    shader->setBool("u_enableDirLight", directionalLight.enabled);
    if (directionalLight.enabled) {
        shader->setVec3("u_dirLightDir", glm::normalize(directionalLight.direction));
        shader->setVec3("u_dirLightColor", directionalLight.color);
        shader->setFloat("u_dirLightIntensity", directionalLight.intensity);
    }
    
    // Point Lights (limit to 8)
    int numPointLights = std::min(static_cast<int>(pointLights.size()), 8);
    shader->setInt("u_numPointLights", numPointLights);
    
    for (int i = 0; i < numPointLights; ++i) {
        std::string index = "[" + std::to_string(i) + "]";
        shader->setVec3("u_pointLightPositions" + index, pointLights[i].position);
        shader->setVec3("u_pointLightColors" + index, pointLights[i].color);
        shader->setFloat("u_pointLightIntensities" + index, pointLights[i].intensity);
        shader->setFloat("u_pointLightRanges" + index, pointLights[i].range);
    }
    
    // IBL Settings
    shader->setBool("u_enableIBL", ibl.enabled);
    shader->setFloat("u_iblIntensity", ibl.intensity);
    
    // Terrain Material Parameters
    shader->setFloat("u_grassHeight", terrain.grassHeight);
    shader->setFloat("u_rockHeight", terrain.rockHeight);
    shader->setFloat("u_snowHeight", terrain.snowHeight);
    
    // POM Parameters
    shader->setBool("u_enablePOM", pom.enabled);
    shader->setFloat("u_pomScale", pom.scale);
    shader->setInt("u_pomMinSamples", pom.minSamples);
    shader->setInt("u_pomMaxSamples", pom.maxSamples);
    shader->setFloat("u_pomSharpen", pom.sharpen);
    
    // Atmospheric & Fog Parameters
    shader->setBool("u_enableAtmosphericFog", atmosphere.enableAtmosphericFog);
    shader->setFloat("u_fogDensity", atmosphere.fogDensity);
    shader->setFloat("u_fogHeightFalloff", atmosphere.fogHeightFalloff);
    shader->setVec3("u_fogColor", atmosphere.fogColor);
    shader->setFloat("u_atmosphericPerspective", atmosphere.atmosphericPerspective);
    shader->setVec3("u_sunDirection", directionalLight.direction);
    
    // Full atmospheric scattering parameters (for atmosphere shader)
    shader->setFloat("u_atmosphereRadius", atmosphere.atmosphereRadius);
    shader->setFloat("u_planetRadius", atmosphere.planetRadius);
    shader->setFloat("u_rayleighCoeff", atmosphere.rayleighCoeff);
    shader->setFloat("u_mieCoeff", atmosphere.mieCoeff);
    shader->setFloat("u_mieG", atmosphere.mieG);
    shader->setFloat("u_sunDiskSize", atmosphere.sunDiskSize);
    shader->setFloat("u_exposure", atmosphere.exposure);
}

void LightingConfig::setupDefaultLights() {
    // Clear existing lights
    pointLights.clear();
    
    // Softer daytime sun
    directionalLight = DirectionalLight(
        glm::vec3(0.3f, -0.7f, -0.2f),  // Direction
        glm::vec3(1.0f, 0.95f, 0.8f),   // Warm white color
        1.2f                             // Reduced intensity
    );
    
    // Add some subtle accent point lights
    addPointLight(PointLight(
        glm::vec3(10.0f, 3.0f, 10.0f),   // Position
        glm::vec3(1.0f, 0.6f, 0.2f),     // Warm orange (campfire)
        8.0f,                            // Reduced intensity
        25.0f                            // Range
    ));
    
    addPointLight(PointLight(
        glm::vec3(-15.0f, 5.0f, -10.0f), // Position
        glm::vec3(0.2f, 0.4f, 1.0f),     // Cool blue
        6.0f,                            // Reduced intensity
        20.0f                            // Range
    ));
    
    // Default terrain and POM settings
    terrain.grassHeight = 2.0f;
    terrain.rockHeight = 8.0f;
    terrain.snowHeight = 12.0f;
    
    pom.enabled = true;
    pom.scale = 0.08f;
    pom.minSamples = 64;
    pom.maxSamples = 128;
    
    ibl.enabled = false;
    ibl.intensity = 0.3f;
    
    std::cout << "Lighting Config: Default daytime lighting setup" << std::endl;
}

void LightingConfig::setupSunsetLighting() {
    pointLights.clear();
    
    // Low, warm sunset sun
    directionalLight = DirectionalLight(
        glm::vec3(0.8f, -0.3f, 0.2f),   // Low angle sun
        glm::vec3(1.0f, 0.4f, 0.1f),    // Orange sunset color
        2.5f                             // Softer intensity
    );
    
    // Rim lighting effects
    addPointLight(PointLight(
        glm::vec3(20.0f, 2.0f, 15.0f),   // Sunset rim light
        glm::vec3(1.0f, 0.3f, 0.0f),     // Deep orange
        25.0f, 30.0f
    ));
    
    addPointLight(PointLight(
        glm::vec3(-10.0f, 1.0f, -5.0f),  // Ambient fill
        glm::vec3(0.4f, 0.2f, 0.6f),     // Purple twilight
        12.0f, 20.0f
    ));
    
    std::cout << "Lighting Config: Dramatic sunset lighting setup" << std::endl;
}

void LightingConfig::setupNightLighting() {
    // Disable or drastically reduce directional light
    directionalLight.enabled = false;
    
    pointLights.clear();
    
    // Multiple colorful point lights for night scene
    addPointLight(PointLight(
        glm::vec3(8.0f, 4.0f, 8.0f),     // Torch 1
        glm::vec3(1.0f, 0.7f, 0.3f),     // Warm torch light
        30.0f, 15.0f
    ));
    
    addPointLight(PointLight(
        glm::vec3(-12.0f, 3.0f, 5.0f),   // Torch 2
        glm::vec3(1.0f, 0.8f, 0.4f),     // Another torch
        25.0f, 18.0f
    ));
    
    addPointLight(PointLight(
        glm::vec3(0.0f, 8.0f, -15.0f),   // Magic crystal
        glm::vec3(0.3f, 0.8f, 1.0f),     // Bright blue magic
        20.0f, 25.0f
    ));
    
    addPointLight(PointLight(
        glm::vec3(15.0f, 2.0f, -8.0f),   // Mystical green
        glm::vec3(0.2f, 1.0f, 0.3f),     // Green magic
        18.0f, 20.0f
    ));
    
    addPointLight(PointLight(
        glm::vec3(-5.0f, 6.0f, 12.0f),   // Purple essence
        glm::vec3(0.8f, 0.2f, 1.0f),     // Purple magic
        22.0f, 16.0f
    ));
    
    std::cout << "Lighting Config: Atmospheric night scene with multiple lights" << std::endl;
}

void LightingConfig::addPointLight(const PointLight& light) {
    if (pointLights.size() < 8) {  // OpenGL array limit
        pointLights.push_back(light);
    } else {
        std::cout << "Warning: Maximum number of point lights (8) reached" << std::endl;
    }
}

void LightingConfig::clearPointLights() {
    pointLights.clear();
}

void LightingConfig::updateTimeOfDay(float deltaTime) {
    if (timeOfDay.animateTimeOfDay) {
        timeOfDay.timeOfDay += deltaTime * timeOfDay.daySpeed;
        
        // Wrap around after 24 hours
        if (timeOfDay.timeOfDay >= 1.0f) {
            timeOfDay.timeOfDay -= 1.0f;
        }
        
        // Update directional light based on time of day
        directionalLight.direction = timeOfDay.getSunDirection();
        directionalLight.color = timeOfDay.getSunColor();
        directionalLight.intensity = timeOfDay.getSunIntensity();
        
        // Adjust fog color based on time of day
        if (timeOfDay.timeOfDay < 0.3f || timeOfDay.timeOfDay > 0.7f) {
            // Dawn/Dusk/Night - warmer, more colorful fog
            float mixFactor = std::clamp(std::abs(timeOfDay.timeOfDay - 0.5f) * 4.0f - 1.0f, 0.0f, 1.0f);
            glm::vec3 nightFog(0.4f, 0.4f, 0.6f);
            glm::vec3 sunsetFog(0.8f, 0.6f, 0.4f);
            atmosphere.fogColor = nightFog + mixFactor * (sunsetFog - nightFog);
        } else {
            // Day - standard blue-ish fog
            atmosphere.fogColor = glm::vec3(0.7f, 0.8f, 0.9f);
        }
    }
}
