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
#include <memory>
#include <vector>
#include "Shader.h"

/**
 * PostProcessor - Handles off-screen rendering and post-processing effects
 * 
 * Implements a flexible post-processing pipeline with:
 * - Off-screen FBO rendering
 * - Bloom (bright pass extraction + Gaussian blur)
 * - Depth of Field (bokeh-style blur based on depth)
 * - Chromatic Aberration (RGB channel displacement)
 */
class PostProcessor {
public:
    /**
     * Post-processing configuration
     */
    struct Config {
        // Bloom settings
        bool enableBloom = true;
        float bloomThreshold = 1.0f;        // Brightness threshold for bloom
        float bloomIntensity = 0.8f;        // Bloom effect strength
        int bloomBlurPasses = 5;            // Number of blur iterations
        
        // Depth of Field settings
        bool enableDOF = true;
        float focusDistance = 10.0f;        // Distance to focus plane
        float focusRange = 5.0f;            // Range around focus that stays sharp
        float bokehRadius = 3.0f;           // Maximum blur radius
        
        // Chromatic Aberration settings
        bool enableChromaticAberration = true;
        float aberrationStrength = 0.5f;   // Aberration displacement amount
        
        // General settings
        float exposure = 1.0f;              // HDR exposure adjustment
        float gamma = 2.2f;                 // Gamma correction
    };

    /**
     * Initialize post-processor with screen dimensions
     */
    PostProcessor(int width, int height);
    
    /**
     * Cleanup resources
     */
    ~PostProcessor();
    
    /**
     * Resize framebuffers (call when window resizes)
     */
    void resize(int width, int height);
    
    /**
     * Begin off-screen rendering (bind main FBO)
     */
    void beginFrame();
    
    /**
     * End off-screen rendering and apply post-processing
     */
    void endFrame();
    
    /**
     * Update configuration
     */
    void setConfig(const Config& config) { m_config = config; }
    const Config& getConfig() const { return m_config; }

private:
    // Screen dimensions
    int m_width, m_height;
    
    // Configuration
    Config m_config;
    
    // Framebuffers
    GLuint m_mainFBO = 0;           // Main scene rendering
    GLuint m_colorTexture = 0;      // Main scene color
    GLuint m_depthTexture = 0;      // Scene depth buffer
    GLuint m_brightTexture = 0;     // Bright areas for bloom
    
    GLuint m_pingPongFBO[2] = {0, 0};   // For ping-pong blur
    GLuint m_pingPongTexture[2] = {0, 0};
    
    // Screen quad for full-screen passes
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;
    
    // Shaders
    std::unique_ptr<Shader> m_brightPassShader;     // Extract bright areas
    std::unique_ptr<Shader> m_blurShader;          // Gaussian blur
    std::unique_ptr<Shader> m_finalShader;         // Final composite with all effects
    
    /**
     * Initialize framebuffers and render targets
     */
    void initFramebuffers();
    
    /**
     * Initialize screen quad geometry
     */
    void initQuad();
    
    /**
     * Load post-processing shaders
     */
    void initShaders();
    
    /**
     * Apply bloom effect (bright pass + blur)
     */
    void applyBloom();
    
    /**
     * Render full-screen quad
     */
    void renderQuad();
    
    /**
     * Cleanup only framebuffers and textures (keep quad VAO)
     */
    void cleanupFramebuffers();
    
    /**
     * Cleanup all OpenGL resources
     */
    void cleanup();
};
