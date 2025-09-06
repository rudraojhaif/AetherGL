#include "HDRLoader.h"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <cmath>

// Include glm for vector math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Static member definitions
GLuint HDRLoader::s_cubeVAO = 0;
GLuint HDRLoader::s_cubeVBO = 0;
bool HDRLoader::s_cubeInitialized = false;

HDRLoader::IBLTextures HDRLoader::loadHDREnvironment(const std::string& filepath) {
    IBLTextures result;
    
    std::cout << "Loading HDR environment: " << filepath << std::endl;
    
    // Load HDR image
    int width, height, channels;
    float* hdrData = loadHDRImage(filepath, width, height, channels);
    
    if (!hdrData) {
        std::cerr << "Failed to load HDR image: " << filepath << std::endl;
        return result;
    }
    
    std::cout << "HDR loaded: " << width << "x" << height << " (" << channels << " channels)" << std::endl;
    
    // Convert to cubemap
    result.environmentMap = equirectangularToCubemap(hdrData, width, height, 512);
    
    if (result.environmentMap == 0) {
        std::cerr << "Failed to convert HDR to cubemap" << std::endl;
        stbi_image_free(hdrData);
        return result;
    }
    
    // For now, use the environment map for all IBL maps (simplified)
    result.irradianceMap = result.environmentMap;
    result.prefilterMap = result.environmentMap;
    result.brdfLUT = generateSimpleBRDFLUT(512);
    
    // Cleanup source data
    stbi_image_free(hdrData);
    
    std::cout << "HDR environment loaded successfully!" << std::endl;
    return result;
}

GLuint HDRLoader::createSkyboxFromHDR(const std::string& filepath, int size) {
    int width, height, channels;
    float* hdrData = loadHDRImage(filepath, width, height, channels);
    
    if (!hdrData) {
        std::cerr << "Failed to load HDR for skybox: " << filepath << std::endl;
        return 0;
    }
    
    GLuint skyboxCubemap = equirectangularToCubemap(hdrData, width, height, size);
    stbi_image_free(hdrData);
    
    return skyboxCubemap;
}

void HDRLoader::cleanup(const IBLTextures& textures) {
    if (textures.environmentMap != 0) {
        glDeleteTextures(1, &textures.environmentMap);
    }
    if (textures.brdfLUT != 0) {
        glDeleteTextures(1, &textures.brdfLUT);
    }
    
    // Cleanup cube mesh
    if (s_cubeInitialized) {
        glDeleteVertexArrays(1, &s_cubeVAO);
        glDeleteBuffers(1, &s_cubeVBO);
        s_cubeInitialized = false;
    }
}

float* HDRLoader::loadHDRImage(const std::string& filepath, int& width, int& height, int& channels) {
    // Enable HDR loading
    stbi_set_flip_vertically_on_load(true);
    
    float* data = stbi_loadf(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "STB Image error: " << stbi_failure_reason() << std::endl;
        return nullptr;
    }
    
    // Ensure we have RGB data
    if (channels < 3) {
        std::cerr << "HDR image must have at least 3 channels (RGB)" << std::endl;
        stbi_image_free(data);
        return nullptr;
    }
    
    return data;
}

GLuint HDRLoader::equirectangularToCubemap(float* hdrData, int width, int height, int cubemapSize) {
    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    
    // Allocate storage for all 6 faces
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                     cubemapSize, cubemapSize, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // CPU-based conversion for simplicity
    std::vector<float> faceData(cubemapSize * cubemapSize * 3);
    
    for (int face = 0; face < 6; ++face) {
        for (int y = 0; y < cubemapSize; ++y) {
            for (int x = 0; x < cubemapSize; ++x) {
                // Convert cubemap coordinates to direction vector
                float u = (2.0f * x / (cubemapSize - 1.0f)) - 1.0f;
                float v = (2.0f * y / (cubemapSize - 1.0f)) - 1.0f;
                
                glm::vec3 dir;
                
                // Calculate direction for each face
                switch (face) {
                    case 0: dir = glm::normalize(glm::vec3( 1.0f, -v, -u)); break; // +X
                    case 1: dir = glm::normalize(glm::vec3(-1.0f, -v,  u)); break; // -X
                    case 2: dir = glm::normalize(glm::vec3( u,  1.0f,  v)); break; // +Y
                    case 3: dir = glm::normalize(glm::vec3( u, -1.0f, -v)); break; // -Y
                    case 4: dir = glm::normalize(glm::vec3( u, -v,  1.0f)); break; // +Z
                    case 5: dir = glm::normalize(glm::vec3(-u, -v, -1.0f)); break; // -Z
                }
                
                // Convert direction to equirectangular coordinates
                float theta = atan2(dir.z, dir.x);
                float phi = acos(dir.y);
                
                float texU = (theta + M_PI) / (2.0f * M_PI);
                float texV = phi / M_PI;
                
                // Sample from HDR texture
                int hdrX = std::min((int)(texU * width), width - 1);
                int hdrY = std::min((int)(texV * height), height - 1);
                
                int hdrIndex = (hdrY * width + hdrX) * 3;
                int faceIndex = (y * cubemapSize + x) * 3;
                
                faceData[faceIndex + 0] = hdrData[hdrIndex + 0];
                faceData[faceIndex + 1] = hdrData[hdrIndex + 1];
                faceData[faceIndex + 2] = hdrData[hdrIndex + 2];
            }
        }
        
        // Upload face data
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F,
                     cubemapSize, cubemapSize, 0, GL_RGB, GL_FLOAT, faceData.data());
    }
    
    return cubemap;
}

GLuint HDRLoader::generateSimpleIrradianceMap(GLuint environmentMap, int size) {
    // For simplicity, just return the same environment map
    // In a full implementation, this would do proper convolution
    return environmentMap;
}

GLuint HDRLoader::generateSimpleBRDFLUT(int size) {
    GLuint brdfLUT;
    glGenTextures(1, &brdfLUT);
    glBindTexture(GL_TEXTURE_2D, brdfLUT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Generate a simple BRDF LUT
    std::vector<float> brdfData(size * size * 2);
    
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float NdotV = (x + 0.5f) / size;
            float roughness = (y + 0.5f) / size;
            
            // Simplified BRDF approximation
            float a = roughness * roughness;
            float k = (a * a) / 2.0f;
            
            int index = (y * size + x) * 2;
            brdfData[index + 0] = k; // Scale
            brdfData[index + 1] = 1.0f - k; // Bias
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, brdfData.data());
    
    return brdfLUT;
}

void HDRLoader::renderUnitCube() {
    if (!s_cubeInitialized) {
        float vertices[] = {
            // Front face
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            
            // Back face
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            
            // Left face
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            
            // Right face
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            
            // Bottom face
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            
            // Top face
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f
        };
        
        glGenVertexArrays(1, &s_cubeVAO);
        glGenBuffers(1, &s_cubeVBO);
        
        glBindVertexArray(s_cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        
        glBindVertexArray(0);
        s_cubeInitialized = true;
    }
    
    glBindVertexArray(s_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
