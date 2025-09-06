#pragma once

#include <glad/glad.h>
#include <string>
#include <memory>

/**
 * HDRLoader - Loads HDR images and generates cubemaps for IBL
 * 
 * This class provides functionality to load HDR images and convert them
 * to cubemaps for image-based lighting. It supports equirectangular to
 * cubemap conversion and generates the necessary IBL maps.
 * 
 * Industry standard features:
 * - HDR image loading (RGBE format)
 * - Equirectangular to cubemap conversion  
 * - Irradiance map generation
 * - Prefiltered environment map generation
 * - BRDF lookup table generation
 */
class HDRLoader {
public:
    /**
     * Load HDR image and generate IBL cubemaps
     * 
     * @param filepath Path to the HDR image file
     * @param size Size of the generated cubemap (e.g., 512, 1024)
     * @return True if successful, false otherwise
     */
    static bool loadHDREnvironment(const std::string& filepath, int size = 512);

    /**
     * Get the generated cubemap textures
     */
    static GLuint getEnvironmentMap() { return s_envCubemap; }
    static GLuint getIrradianceMap() { return s_irradianceMap; }
    static GLuint getPrefilterMap() { return s_prefilterMap; }
    static GLuint getBRDFLUT() { return s_brdfLUT; }

    /**
     * Cleanup resources
     */
    static void cleanup();

private:
    // Generated textures
    static GLuint s_envCubemap;
    static GLuint s_irradianceMap;
    static GLuint s_prefilterMap;
    static GLuint s_brdfLUT;
    
    // Helper functions
    static GLuint loadHDRTexture(const std::string& filepath);
    static GLuint generateCubemap(GLuint hdrTexture, int size);
    static GLuint generateIrradianceMap(GLuint envCubemap, int size = 64);
    static GLuint generatePrefilterMap(GLuint envCubemap, int size = 128);
    static GLuint generateBRDFLUT(int size = 512);
    
    // Shader utilities
    static GLuint createShader(const std::string& vertexSource, const std::string& fragmentSource);
    static void renderCube();
    static void renderQuad();
    
    // Static mesh data
    static GLuint s_cubeVAO;
    static GLuint s_cubeVBO;
    static GLuint s_quadVAO;
    static GLuint s_quadVBO;
};
