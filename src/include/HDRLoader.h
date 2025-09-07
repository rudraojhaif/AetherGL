// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is the property of Rudra Ojha.  
// Redistribution, modification, or use of this code in any project  
// (commercial or non-commercial) is strictly prohibited without  
// prior written consent from the author.
//
// Unauthorized usage will be considered a violation of copyright law.

#pragma once

#include <glad/glad.h>
#include <string>
#include <memory>

/**
 * HDRLoader - Simplified HDR cubemap loader for IBL
 * 
 * Loads HDR images and creates environment cubemaps for realistic lighting.
 * Supports equirectangular HDR images and generates basic IBL textures.
 */
class HDRLoader {
public:
    struct IBLTextures {
        GLuint environmentMap = 0;
        GLuint irradianceMap = 0;
        GLuint prefilterMap = 0;
        GLuint brdfLUT = 0;
        bool isValid() const { return environmentMap != 0; }
    };

    /**
     * Load HDR environment and generate IBL textures
     */
    static IBLTextures loadHDREnvironment(const std::string& filepath);

    /**
     * Create a skybox cubemap from HDR texture
     */
    static GLuint createSkyboxFromHDR(const std::string& filepath, int size = 1024);

    /**
     * Cleanup textures
     */
    static void cleanup(const IBLTextures& textures);

    /**
     * Render a unit cube for cubemap generation
     */
    static void renderUnitCube();

private:
    /**
     * Load HDR image using stb_image
     */
    static float* loadHDRImage(const std::string& filepath, int& width, int& height, int& channels);

    /**
     * Convert equirectangular HDR to cubemap
     */
    static GLuint equirectangularToCubemap(float* hdrData, int width, int height, int cubemapSize);

    /**
     * Generate simple irradiance map (basic diffuse IBL)
     */
    static GLuint generateSimpleIrradianceMap(GLuint environmentMap, int size = 64);

    /**
     * Create basic BRDF LUT
     */
    static GLuint generateSimpleBRDFLUT(int size = 512);

    // Cube mesh data
    static GLuint s_cubeVAO, s_cubeVBO;
    static bool s_cubeInitialized;
};
