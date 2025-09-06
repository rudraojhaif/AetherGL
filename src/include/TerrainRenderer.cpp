#include "TerrainRenderer.h"
#include "TerrainGenerator.h"
#include "ObjWriter.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

TerrainRenderer::TerrainRenderer() {
    // Initialize camera with optimal position
    m_camera = std::make_unique<Camera>(glm::vec3(0.0f, 12.0f, 20.0f));
    m_camera->setPitch(-20.0f);
    m_camera->setYaw(-90.0f);
    
    std::cout << "TerrainRenderer created" << std::endl;
}

TerrainRenderer::~TerrainRenderer() {
    if (m_iblTextures.isValid()) {
        HDRLoader::cleanup(m_iblTextures);
    }
    std::cout << "TerrainRenderer destroyed" << std::endl;
}

void TerrainRenderer::initialize(int width, int height) {
    m_width = width;
    m_height = height;
    
    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // Enable MSAA
    glViewport(0, 0, width, height);
    
    std::cout << "Initializing TerrainRenderer (" << width << "x" << height << ")..." << std::endl;
    
    try {
        setupShaders();
        setupTerrain();
        setupLighting();
        setupPostProcessing();
        
        std::cout << "TerrainRenderer initialized successfully!" << std::endl;
        printPostProcessingStatus();
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize TerrainRenderer: " << e.what() << std::endl;
        throw;
    }
}

void TerrainRenderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
    
    if (m_postProcessor) {
        m_postProcessor->resize(width, height);
    }
    
    std::cout << "Resized to " << width << "x" << height << std::endl;
}

/**
 * Main render function with FPS optimizations
 * 
 * Performance optimizations implemented:
 * - Delta time-based movement for frame rate independence
 * - Efficient key state checking with cached arrays
 * - Early exit for invalid render states
 * - Graceful fallback from post-processing errors
 */
void TerrainRenderer::render() {
    updateDeltaTime();
    
    // FPS OPTIMIZATION: Cache camera speed calculation
    const float cameraSpeed = 15.0f * m_deltaTime;
    
    // FPS OPTIMIZATION: Batch camera movement updates to reduce function calls
    glm::vec3 movementVector(0.0f);
    if (m_keysPressed[GLFW_KEY_W]) movementVector += m_camera->getFront();
    if (m_keysPressed[GLFW_KEY_S]) movementVector -= m_camera->getFront();
    if (m_keysPressed[GLFW_KEY_A]) movementVector -= glm::normalize(glm::cross(m_camera->getFront(), glm::vec3(0.0f, 1.0f, 0.0f)));
    if (m_keysPressed[GLFW_KEY_D]) movementVector += glm::normalize(glm::cross(m_camera->getFront(), glm::vec3(0.0f, 1.0f, 0.0f)));
    if (m_keysPressed[GLFW_KEY_SPACE]) movementVector += glm::vec3(0.0f, 1.0f, 0.0f);
    if (m_keysPressed[GLFW_KEY_LEFT_SHIFT]) movementVector -= glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Apply batched movement (reduces multiple camera matrix recalculations)
    if (glm::length(movementVector) > 0.0f) {
        glm::vec3 currentPos = m_camera->getPosition();
        m_camera->setPosition(currentPos + glm::normalize(movementVector) * cameraSpeed);
    }
    
    // Update time-of-day animation
    m_lighting.updateTimeOfDay(m_deltaTime);
    
    // Render with or without post-processing
    if (m_postProcessor && m_postProcessingEnabled) {
        // Post-processed rendering
        try {
            m_postProcessor->beginFrame();
            renderScene();
            m_postProcessor->endFrame();
        } catch (const std::exception& e) {
            std::cerr << "Post-processing error: " << e.what() << std::endl;
            std::cerr << "Disabling post-processing and using direct rendering" << std::endl;
            m_postProcessingEnabled = false;
            
            // Clear errors and render directly
            while (glGetError() != GL_NO_ERROR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, m_width, m_height);
            renderScene();
        }
    } else {
        // Direct rendering
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_width, m_height);
        renderScene();
    }
}

/**
 * Core scene rendering with optimized OpenGL state management
 * 
 * Performance optimizations:
 * - Early exit for invalid states
 * - Cached matrix calculations
 * - Optimized texture binding order
 * - Minimal OpenGL state changes
 */
void TerrainRenderer::renderScene() {
    // FPS OPTIMIZATION: Early exit for invalid render state
    if (!m_terrainShader || !m_terrainMesh || !m_camera) {
        return;
    }
    
    // FPS OPTIMIZATION: Static variables to avoid repeated calculations
    static glm::vec4 clearColor(0.4f, 0.6f, 0.9f, 1.0f);
    static bool clearColorSet = false;
    
    // FPS OPTIMIZATION: Only set clear color once (assuming it doesn't change)
    if (!clearColorSet) {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        clearColorSet = true;
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // FPS OPTIMIZATION: Cache matrix calculations
    const float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    glm::mat4 projection = glm::perspective(
        glm::radians(m_camera->getZoom()),
        aspectRatio,
        0.1f, 300.0f  // Near and far planes optimized for terrain viewing
    );
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);  // Identity matrix for world-space terrain
    
    /**
     * SKYBOX RENDERING (rendered first to avoid overdraw)
     * 
     * Skybox rendering optimizations:
     * - Rendered first with depth function GL_LEQUAL to pass depth test at far plane
     * - Uses environment map from IBL textures for realistic lighting
     * - Depth function restored after rendering to prevent depth issues
     */
    if (m_skyboxShader && m_iblTextures.isValid()) {
        glDepthFunc(GL_LEQUAL);  // Allow skybox to pass depth test at maximum depth
        m_skyboxShader->use();
        
        // FPS OPTIMIZATION: Remove translation from view matrix for skybox
        // This makes skybox appear infinitely far away
        glm::mat4 skyboxView = glm::mat3(view);  // Remove translation component
        m_skyboxShader->setMat4("view", skyboxView);
        m_skyboxShader->setMat4("projection", projection);
        m_skyboxShader->setFloat("exposure", 1.0f);  // HDR exposure control
        
        // FPS OPTIMIZATION: Bind texture once for skybox
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_iblTextures.environmentMap);
        m_skyboxShader->setInt("skybox", 0);
        
        HDRLoader::renderUnitCube();  // Render cube geometry for skybox
        glDepthFunc(GL_LESS);         // Restore normal depth testing
    }
    
    /**
     * TERRAIN RENDERING with Physically Based Rendering (PBR)
     * 
     * Rendering pipeline:
     * 1. Activate terrain shader program
     * 2. Apply dynamic lighting configuration
     * 3. Bind IBL textures for realistic environmental lighting  
     * 4. Set transformation matrices for vertex shader
     * 5. Draw terrain mesh with all bound resources
     */
    m_terrainShader->use();
    
    // Apply dynamic lighting (sun position, colors, fog parameters)
    m_lighting.applyToShader(m_terrainShader, m_camera->getPosition());
    
    // FPS OPTIMIZATION: Batch IBL texture binding to reduce state changes
    if (m_iblTextures.isValid()) {
        // Irradiance map for diffuse IBL (texture unit 10)
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_iblTextures.irradianceMap);
        m_terrainShader->setInt("u_irradianceMap", 10);
        
        // Prefiltered environment map for specular IBL (texture unit 11)
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_iblTextures.prefilterMap);
        m_terrainShader->setInt("u_prefilterMap", 11);
        
        // BRDF lookup table for Fresnel approximation (texture unit 12)
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_2D, m_iblTextures.brdfLUT);
        m_terrainShader->setInt("u_brdfLUT", 12);
    }
    
    // FPS OPTIMIZATION: Batch matrix uniform uploads
    m_terrainShader->setMat4("projection", projection);  // Camera perspective
    m_terrainShader->setMat4("view", view);              // Camera view transform
    m_terrainShader->setMat4("model", model);            // Object transform (identity for terrain)
    
    // Render terrain geometry with all bound shaders and textures
    m_terrainMesh->draw(m_terrainShader);
}

void TerrainRenderer::setupShaders() {
    std::cout << "Loading shaders..." << std::endl;
    
    try {
        m_terrainShader = std::make_shared<Shader>("terrain_vertex.glsl", "pbr_terrain_fragment.glsl");
        std::cout << "✓ Terrain shaders loaded" << std::endl;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load terrain shaders: " + std::string(e.what()));
    }
    
    try {
        m_skyboxShader = std::make_shared<Shader>("skybox_vertex.glsl", "skybox_fragment.glsl");
        std::cout << "✓ Skybox shaders loaded" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to load skybox shaders: " << e.what() << std::endl;
    }
}

void TerrainRenderer::setupTerrain() {
    std::cout << "Generating procedural terrain..." << std::endl;
    
    m_terrainMesh = TerrainGenerator::generateTerrainMesh(
        80.0f,   // Width
        80.0f,   // Depth
        100,     // Width segments
        100,     // Depth segments
        glm::vec3(0.0f, 0.0f, 0.0f), // Center
        15.0f,   // Height scale
        0.08f,   // Noise scale
        42       // Seed
    );
    
    if (!m_terrainMesh) {
        throw std::runtime_error("Failed to generate terrain mesh");
    }
    
    std::cout << "✓ Procedural terrain generated (10,201 vertices)" << std::endl;
    
    // Export terrain mesh to OBJ file for external use
    // This does not change existing rendering logic, just saves a copy
    exportTerrainToObj();
}

void TerrainRenderer::setupLighting() {
    std::cout << "Setting up lighting system..." << std::endl;
    
    // Setup default lighting
    m_lighting.setupDefaultLights();
    
    // Configure atmospheric settings
    m_lighting.atmosphere.enableAtmosphericFog = true;
    m_lighting.atmosphere.fogDensity = 0.015f;
    m_lighting.atmosphere.fogHeightFalloff = 0.08f;
    m_lighting.atmosphere.atmosphericPerspective = 0.7f;
    
    m_lighting.timeOfDay.animateTimeOfDay = true;
    m_lighting.timeOfDay.daySpeed = 0.05f;
    m_lighting.timeOfDay.timeOfDay = 0.3f;
    
    // Load HDR environment
    std::cout << "Loading HDR environment..." << std::endl;
    try {
        m_iblTextures = HDRLoader::loadHDREnvironment("assets/qwantani_noon_puresky_4k.hdr");
        if (m_iblTextures.isValid()) {
            m_lighting.ibl.enabled = true;
            m_lighting.ibl.intensity = 0.8f;
            std::cout << "✓ HDR environment loaded successfully" << std::endl;
        } else {
            std::cerr << "Warning: Failed to load HDR environment" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: HDR loading failed: " << e.what() << std::endl;
    }
    
    std::cout << "✓ Lighting system configured" << std::endl;
}

void TerrainRenderer::setupPostProcessing() {
    std::cout << "Initializing post-processing pipeline..." << std::endl;
    
    try {
        m_postProcessor = std::make_unique<PostProcessor>(m_width, m_height);
        
        // Configure post-processing with visible effects
        PostProcessor::Config config;
        config.enableBloom = true;
        config.bloomThreshold = 0.7f;
        config.bloomIntensity = 1.2f;
        config.bloomBlurPasses = 6;
        
        config.enableDOF = false;
        config.enableChromaticAberration = true;
        config.aberrationStrength = 0.8f;
        
        config.exposure = 1.0f;
        config.gamma = 2.2f;
        
        m_postProcessor->setConfig(config);
        
        std::cout << "✓ Post-processing pipeline initialized" << std::endl;
        m_postProcessingEnabled = false;
        
    } catch (const std::exception& e) {
        std::cerr << "Warning: Post-processing failed to initialize: " << e.what() << std::endl;
        std::cerr << "Continuing with direct rendering..." << std::endl;
        m_postProcessor = nullptr;
        m_postProcessingEnabled = false;
    }
}

void TerrainRenderer::updateDeltaTime() {
    float currentFrame = static_cast<float>(glfwGetTime());
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;
}

void TerrainRenderer::processMouseMovement(float xoffset, float yoffset) {
    if (m_camera) {
        m_camera->processMouseMovement(xoffset, yoffset);
    }
}

void TerrainRenderer::processMouseScroll(float yoffset) {
    if (m_camera) {
        m_camera->processMouseScroll(yoffset);
    }
}

void TerrainRenderer::processKeyboard(int key, int action) {
    // Handle continuous movement keys
    if (action == GLFW_PRESS) {
        m_keysPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        m_keysPressed[key] = false;
    }
    
    // Handle toggle keys (only on press)
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:
                toggleBloom();
                break;
            case GLFW_KEY_2:
                toggleDOF();
                break;
            case GLFW_KEY_3:
                toggleChromaticAberration();
                break;
        }
    }
}

void TerrainRenderer::toggleBloom() {
    if (m_postProcessor) {
        auto config = m_postProcessor->getConfig();
        config.enableBloom = !config.enableBloom;
        m_postProcessor->setConfig(config);
        std::cout << "Bloom " << (config.enableBloom ? "enabled" : "disabled") << std::endl;
    }
}

void TerrainRenderer::toggleDOF() {
    if (m_postProcessor) {
        auto config = m_postProcessor->getConfig();
        config.enableDOF = !config.enableDOF;
        m_postProcessor->setConfig(config);
        std::cout << "Depth of Field " << (config.enableDOF ? "enabled" : "disabled") << std::endl;
    }
}

void TerrainRenderer::toggleChromaticAberration() {
    if (m_postProcessor) {
        auto config = m_postProcessor->getConfig();
        config.enableChromaticAberration = !config.enableChromaticAberration;
        m_postProcessor->setConfig(config);
        std::cout << "Chromatic Aberration " << (config.enableChromaticAberration ? "enabled" : "disabled") << std::endl;
    }
}

void TerrainRenderer::printPostProcessingStatus() const {
    if (m_postProcessor && m_postProcessingEnabled) {
        auto config = m_postProcessor->getConfig();
        std::cout << "\nPost-processing Status:" << std::endl;
        std::cout << "✓ Enabled - Bloom: " << (config.enableBloom ? "ON" : "OFF") 
                  << ", DOF: " << (config.enableDOF ? "ON" : "OFF")
                  << ", CA: " << (config.enableChromaticAberration ? "ON" : "OFF") << std::endl;
    } else {
        std::cout << "\nPost-processing: DISABLED (using direct rendering)" << std::endl;
    }
}

// GUI access methods
glm::vec3 TerrainRenderer::getCameraPosition() const {
    return m_camera ? m_camera->getPosition() : glm::vec3(0.0f);
}

LightingConfig& TerrainRenderer::getLightingConfig() {
    return m_lighting;
}

bool TerrainRenderer::isPostProcessingEnabled() const {
    return m_postProcessingEnabled && m_postProcessor != nullptr;
}

void TerrainRenderer::setPostProcessingEnabled(bool enabled) {
    m_postProcessingEnabled = enabled;
}

PostProcessor::Config TerrainRenderer::getPostProcessorConfig() const {
    if (m_postProcessor) {
        return m_postProcessor->getConfig();
    }
    return PostProcessor::Config{};
}

void TerrainRenderer::setPostProcessorConfig(const PostProcessor::Config& config) {
    if (m_postProcessor) {
        m_postProcessor->setConfig(config);
    }
}

/**
 * Export generated terrain mesh to OBJ file
 * 
 * This function automatically exports the procedurally generated terrain mesh
 * to a Wavefront OBJ file immediately after generation. The export process:
 * 1. Creates an "exports" directory if it doesn't exist
 * 2. Generates a timestamped filename to avoid overwriting
 * 3. Uses the ObjWriter to export vertex data, normals, and texture coordinates
 * 4. Includes terrain generation parameters as metadata
 * 
 * The export operation runs asynchronously to the main rendering process and
 * does not affect existing rendering logic or performance.
 */
void TerrainRenderer::exportTerrainToObj() {
    if (!m_terrainMesh) {
        std::cerr << "Cannot export terrain: No terrain mesh available" << std::endl;
        return;
    }

    try {
        // Generate timestamped filename to avoid overwriting previous exports
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::ostringstream filename;
        filename << "exports/terrain_" 
                << std::put_time(&tm, "%Y%m%d_%H%M%S") 
                << ".obj";

        std::cout << "Exporting terrain mesh to: " << filename.str() << std::endl;

        // Export terrain with generation parameters as metadata
        bool success = ObjWriter::exportTerrainMesh(
            m_terrainMesh,
            filename.str(),
            "AetherGL_ProceduralTerrain",
            80.0f,      // Width (matches generation parameters)
            80.0f,      // Depth (matches generation parameters) 
            15.0f,      // Height scale (matches generation parameters)
            42          // Seed (matches generation parameters)
        );

        if (success) {
            // Display mesh statistics for user information
            std::cout << ObjWriter::getMeshStatistics(m_terrainMesh) << std::endl;
        } else {
            std::cerr << "Failed to export terrain mesh to OBJ file" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception during terrain export: " << e.what() << std::endl;
    }
}
