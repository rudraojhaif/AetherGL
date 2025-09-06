#include "PostProcessor.h"
#include <iostream>

PostProcessor::PostProcessor(int width, int height) 
    : m_width(width), m_height(height) {
    
    // Initialize OpenGL resources
    initQuad();
    initShaders();
    initFramebuffers();
    
    std::cout << "PostProcessor initialized with " << width << "x" << height << std::endl;
}

PostProcessor::~PostProcessor() {
    cleanup();
}

void PostProcessor::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    if (width <= 0 || height <= 0) {
        std::cerr << "Invalid resize dimensions: " << width << "x" << height << std::endl;
        return;
    }
    
    m_width = width;
    m_height = height;
    
    // Clear any OpenGL errors before resize
    while (glGetError() != GL_NO_ERROR);
    
    // Save current OpenGL state
    GLint currentFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    
    try {
        // Recreate only framebuffers with new dimensions (keep quad VAO)
        cleanupFramebuffers();
        initFramebuffers();
        
        // Reinitialize quad if it was destroyed during cleanup
        if (m_quadVAO == 0) {
            std::cout << "Reinitializing quad after resize..." << std::endl;
            initQuad();
        }
        
        std::cout << "PostProcessor resized to " << width << "x" << height << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during PostProcessor resize: " << e.what() << std::endl;
        
        // Try to restore previous state
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        return;
    }
    
    // Check for errors after resize
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error during PostProcessor resize: " << error << std::endl;
        
        // Try to restore previous state
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    }
    
    // Restore framebuffer binding
    glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
}

void PostProcessor::beginFrame() {
    // Check if framebuffer is valid before binding
    if (m_mainFBO == 0) {
        std::cerr << "Warning: Main FBO not initialized in beginFrame!" << std::endl;
        return;
    }
    
    // Bind main FBO for off-screen rendering
    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFBO);
    
    // Verify framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete in beginFrame: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Fallback to default framebuffer
        return;
    }
    
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessor::endFrame() {
    // Apply post-processing pipeline and render to screen
    
    // Safety check: Ensure main FBO is valid
    if (m_mainFBO == 0 || m_colorTexture == 0) {
        std::cerr << "Warning: PostProcessor not properly initialized for endFrame" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_width, m_height);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    
    // Ensure we're working with a clean state
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Step 1: Extract bright areas for bloom (if enabled)
    if (m_config.enableBloom && m_brightPassShader && m_blurShader) {
        try {
            applyBloom();
        } catch (const std::exception& e) {
            std::cerr << "Bloom processing failed: " << e.what() << std::endl;
            // Continue without bloom
        }
    }
    
    // Step 2: Final composite pass with all effects
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Ensure default framebuffer
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Clear any pending OpenGL errors before final pass
    while (glGetError() != GL_NO_ERROR);
    
    // Disable depth testing for final pass
    glDisable(GL_DEPTH_TEST);
    
    // Debug: Check if shader is valid
    if (!m_finalShader) {
        std::cerr << "Final shader is null!" << std::endl;
        return;
    }
    
    m_finalShader->use();
    
    // Check for errors after using final shader
    GLenum shaderError = glGetError();
    if (shaderError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after using final shader: " << shaderError << std::endl;
    }
    
    // Debug: Check if main texture is valid
    if (m_colorTexture == 0) {
        std::cerr << "Color texture is invalid!" << std::endl;
        return;
    }
    
    static bool debugPrinted = false;
    if (!debugPrinted) {
        std::cout << "Rendering post-process with textures: color=" << m_colorTexture 
                  << ", depth=" << m_depthTexture << std::endl;
        debugPrinted = true;
    }
    
    // Bind main scene texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    m_finalShader->setInt("u_sceneTexture", 0);
    
    // Bind depth texture for DOF
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    m_finalShader->setInt("u_depthTexture", 1);
    
    // Always bind bloom texture (black texture when disabled)
    glActiveTexture(GL_TEXTURE2);
    if (m_config.enableBloom && m_pingPongTexture[0] != 0) {
        glBindTexture(GL_TEXTURE_2D, m_pingPongTexture[0]); // Final blur result
    } else {
        // Create a persistent black texture for when bloom is disabled
        static GLuint blackTexture = 0;
        if (blackTexture == 0) {
            glGenTextures(1, &blackTexture);
            glBindTexture(GL_TEXTURE_2D, blackTexture);
            unsigned char blackPixel[4] = {0, 0, 0, 0};
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackPixel);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            glBindTexture(GL_TEXTURE_2D, blackTexture);
        }
    }
    m_finalShader->setInt("u_bloomTexture", 2);
    
    // Set post-processing uniforms
    m_finalShader->setBool("u_enableBloom", m_config.enableBloom);
    m_finalShader->setFloat("u_bloomIntensity", m_config.bloomIntensity);
    
    m_finalShader->setBool("u_enableDOF", m_config.enableDOF);
    m_finalShader->setFloat("u_focusDistance", m_config.focusDistance);
    m_finalShader->setFloat("u_focusRange", m_config.focusRange);
    m_finalShader->setFloat("u_bokehRadius", m_config.bokehRadius);
    
    m_finalShader->setBool("u_enableChromaticAberration", m_config.enableChromaticAberration);
    m_finalShader->setFloat("u_aberrationStrength", m_config.aberrationStrength);
    
    m_finalShader->setFloat("u_exposure", m_config.exposure);
    m_finalShader->setFloat("u_gamma", m_config.gamma);
    
    // Check for errors before rendering quad
    GLenum preRenderError = glGetError();
    if (preRenderError != GL_NO_ERROR) {
        std::cerr << "OpenGL error before rendering quad: " << preRenderError << std::endl;
    }
    
    // Render full-screen quad
    renderQuad();
    
    // Check for errors after rendering quad
    GLenum postRenderError = glGetError();
    if (postRenderError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after rendering quad: " << postRenderError << std::endl;
    }
    
    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Check for errors after re-enabling depth test
    GLenum depthError = glGetError();
    if (depthError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after re-enabling depth test: " << depthError << std::endl;
    }
    
    // Debug: Check for errors after rendering with detailed tracking
    GLenum finalError = glGetError();
    if (finalError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after final pass: " << finalError << std::endl;
        std::cerr << "PostProcessor state - FBO: " << m_mainFBO << ", ColorTex: " << m_colorTexture 
                  << ", DepthTex: " << m_depthTexture << std::endl;
        std::cerr << "Dimensions: " << m_width << "x" << m_height << std::endl;
        
        // Check current OpenGL state
        GLint currentFBO = 0, currentTexture = 0, currentVAO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        
        std::cerr << "Current GL state - FBO: " << currentFBO 
                  << ", Texture: " << currentTexture << ", VAO: " << currentVAO << std::endl;
                  
        // Clear the error so it doesn't accumulate
        while (glGetError() != GL_NO_ERROR);
    }
}

void PostProcessor::initFramebuffers() {
    // Main FBO for scene rendering
    glGenFramebuffers(1, &m_mainFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFBO);
    
    // Color texture (HDR format with fallback)
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    
    // Try HDR format first, fallback to standard RGBA if not supported
    GLenum internalFormat = GL_RGBA16F;
    GLenum format = GL_RGBA;
    GLenum type = GL_FLOAT;
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, type, nullptr);
    
    // Check if texture creation was successful
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "HDR texture format not supported, falling back to RGBA8..." << std::endl;
        // Fallback to standard RGBA8
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    } else {
        std::cout << "HDR color texture created successfully" << std::endl;
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    
    // Depth texture
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    
    // Check framebuffer completeness
    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Main framebuffer not complete! Status: " << fbStatus << std::endl;
        switch (fbStatus) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                std::cerr << "  - Incomplete attachment" << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                std::cerr << "  - Missing attachment" << std::endl;
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                std::cerr << "  - Unsupported framebuffer format" << std::endl;
                break;
            default:
                std::cerr << "  - Unknown framebuffer error" << std::endl;
        }
    } else {
        std::cout << "Main framebuffer created successfully" << std::endl;
    }
    
    // Ping-pong FBOs for bloom blur - with proper error handling
    glGenFramebuffers(2, m_pingPongFBO);
    glGenTextures(2, m_pingPongTexture);
    
    bool pingPongSuccess = true;
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingPongFBO[i]);
        
        glBindTexture(GL_TEXTURE_2D, m_pingPongTexture[i]);
        
        // Try HDR format first, fallback to RGBA8
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        GLenum texError = glGetError();
        if (texError != GL_NO_ERROR) {
            std::cout << "HDR format failed for ping-pong texture " << i << ", using RGBA8" << std::endl;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingPongTexture[i], 0);
        
        GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Ping-pong framebuffer " << i << " not complete! Status: " << fbStatus << std::endl;
            pingPongSuccess = false;
        } else {
            std::cout << "Ping-pong framebuffer " << i << " created successfully" << std::endl;
        }
    }
    
    if (!pingPongSuccess) {
        std::cerr << "Warning: Bloom may not work properly due to ping-pong FBO issues" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::initQuad() {
    // Full-screen quad vertices
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void PostProcessor::initShaders() {
    try {
        // Load post-processing shaders
        m_brightPassShader = std::make_unique<Shader>("shaders/quad_vertex.glsl", "shaders/bright_pass_fragment.glsl");
        m_blurShader = std::make_unique<Shader>("shaders/quad_vertex.glsl", "shaders/blur_fragment.glsl");
        m_finalShader = std::make_unique<Shader>("shaders/quad_vertex.glsl", "shaders/final_postprocess_fragment.glsl");
        
        std::cout << "Post-processing shaders loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load post-processing shaders: " << e.what() << std::endl;
    }
}

void PostProcessor::applyBloom() {
    // Step 1: Extract bright areas
    glBindFramebuffer(GL_FRAMEBUFFER, m_pingPongFBO[0]);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);
    
    m_brightPassShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    m_brightPassShader->setInt("u_sceneTexture", 0);
    m_brightPassShader->setFloat("u_threshold", m_config.bloomThreshold);
    
    renderQuad();
    
    // Step 2: Blur bright areas with ping-pong technique
    bool horizontal = true;
    int blurAmount = m_config.bloomBlurPasses;
    
    m_blurShader->use();
    for (int i = 0; i < blurAmount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingPongFBO[horizontal ? 1 : 0]);
        glViewport(0, 0, m_width, m_height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        m_blurShader->setBool("u_horizontal", horizontal);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_pingPongTexture[horizontal ? 0 : 1]);
        m_blurShader->setInt("u_image", 0);
        
        renderQuad();
        horizontal = !horizontal;
    }
}

void PostProcessor::renderQuad() {
    if (m_quadVAO == 0) {
        std::cerr << "Error: Quad VAO not initialized!" << std::endl;
        return;
    }
    
    glBindVertexArray(m_quadVAO);
    
    // Check for errors after binding VAO
    GLenum vaoError = glGetError();
    if (vaoError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after binding quad VAO: " << vaoError << std::endl;
    }
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Check for errors after draw call
    GLenum drawError = glGetError();
    if (drawError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after draw arrays: " << drawError << std::endl;
    }
    
    glBindVertexArray(0);
}

void PostProcessor::cleanupFramebuffers() {
    // Save current OpenGL state
    GLint currentFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    
    // Unbind framebuffers before deleting
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Delete framebuffers
    if (m_mainFBO) {
        glDeleteFramebuffers(1, &m_mainFBO);
        m_mainFBO = 0;
    }
    
    if (m_pingPongFBO[0] || m_pingPongFBO[1]) {
        GLuint fbos[2] = {m_pingPongFBO[0], m_pingPongFBO[1]};
        glDeleteFramebuffers(2, fbos);
        m_pingPongFBO[0] = m_pingPongFBO[1] = 0;
    }
    
    // Unbind textures before deleting
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Delete textures
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    
    if (m_depthTexture) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
    
    if (m_pingPongTexture[0] || m_pingPongTexture[1]) {
        GLuint textures[2] = {m_pingPongTexture[0], m_pingPongTexture[1]};
        glDeleteTextures(2, textures);
        m_pingPongTexture[0] = m_pingPongTexture[1] = 0;
    }
    
    // Clear any errors that may have occurred during cleanup
    while (glGetError() != GL_NO_ERROR);
    
    // Restore framebuffer binding (if it was valid and still exists)
    if (currentFBO > 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    }
}

void PostProcessor::cleanup() {
    // Save current OpenGL state
    GLint currentFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    
    // Unbind framebuffers before deleting
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Delete framebuffers
    if (m_mainFBO) {
        glDeleteFramebuffers(1, &m_mainFBO);
        m_mainFBO = 0;
    }
    
    if (m_pingPongFBO[0] || m_pingPongFBO[1]) {
        GLuint fbos[2] = {m_pingPongFBO[0], m_pingPongFBO[1]};
        glDeleteFramebuffers(2, fbos);
        m_pingPongFBO[0] = m_pingPongFBO[1] = 0;
    }
    
    // Unbind textures before deleting
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Delete textures
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    
    if (m_depthTexture) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
    
    if (m_pingPongTexture[0] || m_pingPongTexture[1]) {
        GLuint textures[2] = {m_pingPongTexture[0], m_pingPongTexture[1]};
        glDeleteTextures(2, textures);
        m_pingPongTexture[0] = m_pingPongTexture[1] = 0;
    }
    
    // Unbind VAO before deleting
    glBindVertexArray(0);
    
    // Delete quad VAO/VBO
    if (m_quadVAO) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    
    if (m_quadVBO) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    
    // Clear any errors that may have occurred during cleanup
    while (glGetError() != GL_NO_ERROR);
    
    // Restore framebuffer binding (if it was valid and still exists)
    if (currentFBO > 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    }
}
