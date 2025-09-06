#include <glad/glad.h>  // Must be first to avoid OpenGL header conflicts
#include "TerrainViewer.h"
#include "TerrainGenerator.h"
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QElapsedTimer>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

TerrainViewer::TerrainViewer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_updateTimer(new QTimer(this))
    , m_lastFrameTime(0)
{
    // Enable focus for keyboard input
    setFocusPolicy(Qt::StrongFocus);
    
    // Initialize viewport dimensions
    m_viewportWidth = 800;
    m_viewportHeight = 600;
    
    // Setup update timer for smooth animation
    connect(m_updateTimer, &QTimer::timeout, this, &TerrainViewer::updateScene);
    m_updateTimer->start(16); // ~60 FPS
    
    // Initialize camera with optimal position for terrain and post-processing viewing
    m_camera = std::make_unique<Camera>(glm::vec3(0.0f, 12.0f, 20.0f));
    // Point camera downward to see terrain better
    m_camera->setPitch(-20.0f);  // Look down at terrain
    m_camera->setYaw(-90.0f);    // Face forward (-Z direction)
}

TerrainViewer::~TerrainViewer()
{
    makeCurrent();
    
    // Cleanup HDR resources
    if (m_iblTextures.isValid()) {
        HDRLoader::cleanup(m_iblTextures);
    }
    
    doneCurrent();
}

void TerrainViewer::initializeGL()
{
    // Make sure the context is current
    makeCurrent();
    
    // Initialize GLAD for OpenGL function loading
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }
    
    // Print OpenGL info for debugging
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    
    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Set initial viewport
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    std::cout << "Initial viewport set to: " << m_viewportWidth << "x" << m_viewportHeight << std::endl;
    
    try {
        setupShaders();
        setupTerrain();
        setupLighting();
        
        // Load HDR environment
        std::cout << "Loading HDR environment..." << std::endl;
        m_iblTextures = HDRLoader::loadHDREnvironment("assets/qwantani_noon_puresky_4k.hdr");
        
        if (!m_iblTextures.isValid()) {
            std::cerr << "Warning: Failed to load HDR environment. IBL will be disabled." << std::endl;
        } else {
            std::cout << "HDR environment loaded successfully!" << std::endl;
        }
        
        // Initialize post-processing pipeline with proper error handling
        std::cout << "Initializing industry-standard post-processing pipeline..." << std::endl;
        try {
            m_postProcessor = std::make_unique<PostProcessor>(m_viewportWidth, m_viewportHeight);
            
            // Configure post-processing with visible defaults
            PostProcessor::Config postConfig;
            postConfig.enableBloom = true;
            postConfig.bloomThreshold = 0.7f;      // Lower threshold for more bloom
            postConfig.bloomIntensity = 1.2f;      // Higher intensity for visibility
            postConfig.bloomBlurPasses = 6;        // More blur passes for quality
            
            postConfig.enableDOF = false;          // Start disabled for stability
            postConfig.focusDistance = 15.0f;
            postConfig.focusRange = 5.0f;
            postConfig.bokehRadius = 3.0f;
            
            postConfig.enableChromaticAberration = true;  // Enable for visibility
            postConfig.aberrationStrength = 0.8f;        // More visible effect
            
            postConfig.exposure = 1.0f;             // Neutral exposure
            postConfig.gamma = 2.2f;                // Standard gamma
            
            m_postProcessor->setConfig(postConfig);
            std::cout << "Post-processing initialized successfully!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Post-processing initialization failed: " << e.what() << std::endl;
            std::cerr << "Falling back to direct rendering" << std::endl;
            m_postProcessor = nullptr;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error initializing OpenGL resources: " << e.what() << std::endl;
    }
    
    std::cout << "TerrainViewer initialized!" << std::endl;
}

void TerrainViewer::paintGL()
{
    // Clear any previous OpenGL errors
    while(glGetError() != GL_NO_ERROR);
    
    // Calculate delta time
    static QElapsedTimer timer;
    if (m_lastFrameTime == 0) {
        timer.start();
        m_lastFrameTime = timer.elapsed();
    }
    qint64 currentTime = timer.elapsed();
    m_deltaTime = (currentTime - m_lastFrameTime) / 1000.0f;
    m_lastFrameTime = currentTime;
    
    // Update time-of-day animation
    m_lighting.updateTimeOfDay(m_deltaTime);
    
    // Industry-standard post-processing pipeline with comprehensive error detection
    static bool postProcessingFailed = false;
    
    if (m_postProcessor && !postProcessingFailed) {
        // Clear any pending OpenGL errors before post-processing
        while (glGetError() != GL_NO_ERROR);
        
        try {
            m_postProcessor->beginFrame();
            
            // Check for errors after beginFrame
            GLenum beginError = glGetError();
            if (beginError != GL_NO_ERROR) {
                throw std::runtime_error("BeginFrame failed with OpenGL error: " + std::to_string(beginError));
            }
            
            renderScene();
            
            // Check for errors after scene rendering
            GLenum sceneError = glGetError();
            if (sceneError != GL_NO_ERROR) {
                throw std::runtime_error("Scene rendering failed with OpenGL error: " + std::to_string(sceneError));
            }
            
            m_postProcessor->endFrame();
            
            // Check for errors after endFrame
            GLenum endError = glGetError();
            if (endError != GL_NO_ERROR) {
                throw std::runtime_error("EndFrame failed with OpenGL error: " + std::to_string(endError));
            }
            
        } catch (const std::exception& e) {
            std::cerr << "\n=== POST-PROCESSING ERROR DETECTED ===" << std::endl;
            std::cerr << "Error: " << e.what() << std::endl;
            std::cerr << "Permanently disabling post-processing and falling back to direct rendering" << std::endl;
            std::cerr << "==========================================\n" << std::endl;
            
            postProcessingFailed = true;
            m_postProcessor = nullptr;
            
            // Clear any OpenGL errors and render directly
            while (glGetError() != GL_NO_ERROR);
            renderScene();
        }
    } else {
        // Direct rendering fallback (either no post-processor or it failed)
        renderScene();
        
        static bool fallbackMessageShown = false;
        if (!fallbackMessageShown && postProcessingFailed) {
            std::cout << "Using direct rendering mode (post-processing disabled due to errors)" << std::endl;
            fallbackMessageShown = true;
        }
    }
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        static int errorCount = 0;
        if (errorCount < 5) { // Only print first 5 errors to avoid spam
            std::cerr << "OpenGL error in paintGL: " << error << std::endl;
            errorCount++;
        }
    }
}

void TerrainViewer::resizeGL(int w, int h)
{
    m_viewportWidth = w;
    m_viewportHeight = h;
    
    // Set viewport immediately on resize
    glViewport(0, 0, w, h);
    
    std::cout << "Viewport resized to: " << w << "x" << h << std::endl;
}

void TerrainViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_mouseCaptured) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
    }
}

void TerrainViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (m_camera) {
        if (m_mouseCaptured) {
            // In mouse capture mode, use relative movement from center
            QPoint center = rect().center();
            QPoint delta = event->pos() - center;
            
            if (delta.x() != 0 || delta.y() != 0) {
                m_camera->processMouseMovement(delta.x() * 0.1f, -delta.y() * 0.1f);
                
                // Reset cursor to center
                QCursor::setPos(mapToGlobal(center));
                update();
            }
        } else if (m_mousePressed) {
            // In click-drag mode
            QPoint delta = event->pos() - m_lastMousePos;
            m_camera->processMouseMovement(delta.x(), -delta.y()); // Invert Y
            m_lastMousePos = event->pos();
            update(); // Trigger repaint
        }
    }
}

void TerrainViewer::keyPressEvent(QKeyEvent *event)
{
    if (!m_camera) return;
    
    const float speed = 0.1f; // Adjust as needed
    
    switch (event->key()) {
        case Qt::Key_W:
            m_camera->processKeyboard(CameraMovement::FORWARD, speed);
            break;
        case Qt::Key_S:
            m_camera->processKeyboard(CameraMovement::BACKWARD, speed);
            break;
        case Qt::Key_A:
            m_camera->processKeyboard(CameraMovement::LEFT, speed);
            break;
        case Qt::Key_D:
            m_camera->processKeyboard(CameraMovement::RIGHT, speed);
            break;
        case Qt::Key_Space:
            m_camera->processKeyboard(CameraMovement::UP, speed);
            break;
        case Qt::Key_Shift:
            m_camera->processKeyboard(CameraMovement::DOWN, speed);
            break;
        case Qt::Key_M: {
            // Toggle mouse capture mode (simplified)
            m_mouseCaptured = !m_mouseCaptured;
            
            if (m_mouseCaptured) {
                // Enter mouse capture mode
                setCursor(Qt::BlankCursor);
                setMouseTracking(true);
                QCursor::setPos(mapToGlobal(rect().center()));
                std::cout << "Mouse capture enabled - WASD to move, mouse to look, M to exit" << std::endl;
            } else {
                // Exit mouse capture mode
                setCursor(Qt::ArrowCursor);
                setMouseTracking(false);
                m_mousePressed = false;
                std::cout << "Mouse capture disabled - left-click and drag to look around, press M to enable" << std::endl;
            }
            break;
        }
    }
    update(); // Trigger repaint
}

void TerrainViewer::wheelEvent(QWheelEvent *event)
{
    if (m_camera) {
        m_camera->processMouseScroll(event->angleDelta().y() / 120.0f);
        update(); // Trigger repaint
    }
}

void TerrainViewer::updateScene()
{
    update(); // Trigger paintGL
}

void TerrainViewer::setPostProcessorConfig(const PostProcessor::Config& config)
{
    if (m_postProcessor) {
        m_postProcessor->setConfig(config);
    }
}

PostProcessor::Config TerrainViewer::getPostProcessorConfig() const
{
    if (m_postProcessor) {
        return m_postProcessor->getConfig();
    }
    return PostProcessor::Config{};
}

void TerrainViewer::setupTerrain()
{
    // Generate procedural terrain mesh
    m_terrainMesh = TerrainGenerator::generateTerrainMesh(
        80.0f,  // Width
        80.0f,  // Depth  
        100,    // Width segments
        100,    // Depth segments
        glm::vec3(0.0f, 0.0f, 0.0f),  // Center position
        15.0f,  // Height scale
        0.08f,  // Noise scale
        42      // Seed
    );
    
    if (!m_terrainMesh) {
        throw std::runtime_error("Failed to generate terrain mesh!");
    }
    
    std::cout << "Procedural terrain generated successfully!" << std::endl;
}

void TerrainViewer::setupShaders()
{
    try {
        // Load PBR terrain shaders
        m_terrainShader = std::make_shared<Shader>("terrain_vertex.glsl", "pbr_terrain_fragment.glsl");
        std::cout << "Terrain shaders loaded successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load terrain shaders: " << e.what() << std::endl;
        std::cerr << "This will cause rendering issues!" << std::endl;
    }
    
    try {
        // Load skybox shader
        m_skyboxShader = std::make_shared<Shader>("skybox_vertex.glsl", "skybox_fragment.glsl");
        std::cout << "Skybox shaders loaded successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load skybox shaders: " << e.what() << std::endl;
        std::cerr << "Skybox will not render!" << std::endl;
    }
    
    std::cout << "Shader loading complete!" << std::endl;
}

void TerrainViewer::setupLighting()
{
    // Setup default lighting
    m_lighting.setupDefaultLights();
    
    // Configure atmospheric and IBL settings
    m_lighting.atmosphere.enableAtmosphericFog = true;
    m_lighting.atmosphere.fogDensity = 0.015f;
    m_lighting.atmosphere.fogHeightFalloff = 0.08f;
    m_lighting.atmosphere.atmosphericPerspective = 0.7f;
    
    m_lighting.ibl.enabled = m_iblTextures.isValid();
    m_lighting.ibl.intensity = 0.8f;
    
    m_lighting.timeOfDay.animateTimeOfDay = true;
    m_lighting.timeOfDay.daySpeed = 0.05f;
    m_lighting.timeOfDay.timeOfDay = 0.3f;
    
    // Enhanced POM for atmospheric scenes
    m_lighting.pom.scale = 0.1f;
    m_lighting.pom.maxSamples = 48;
    
    // Biome height adjustments
    m_lighting.terrain.grassHeight = 1.0f;
    m_lighting.terrain.rockHeight = 6.0f;
    m_lighting.terrain.snowHeight = 10.0f;
    
    std::cout << "Lighting configuration initialized!" << std::endl;
}

void TerrainViewer::renderScene()
{
    // Clear framebuffer - use a nice sky blue color
    glClearColor(0.4f, 0.6f, 0.9f, 1.0f);  // Light blue sky background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Debug output (only print occasionally to avoid spam)
    static int frameCount = 0;
    if (frameCount % 60 == 0) {  // Print every 60 frames
        std::cout << "Frame " << frameCount << ": Camera pos (" 
                  << m_camera->getPosition().x << ", " 
                  << m_camera->getPosition().y << ", " 
                  << m_camera->getPosition().z << "), viewport: " 
                  << m_viewportWidth << "x" << m_viewportHeight << std::endl;
    }
    frameCount++;
    
    // Check if terrain shader is valid
    if (!m_terrainShader) {
        std::cerr << "Terrain shader is null! Cannot render." << std::endl;
        return;
    }
    
    // Check if terrain mesh exists
    if (!m_terrainMesh) {
        std::cerr << "Terrain mesh is null! Cannot render terrain." << std::endl;
        return;
    }
    
    // Use terrain shader
    m_terrainShader->use();
    
    // Apply all lighting settings to shader
    m_lighting.applyToShader(m_terrainShader, m_camera->getPosition());
    
    // Bind IBL textures if available
    if (m_iblTextures.isValid()) {
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_iblTextures.irradianceMap);
        m_terrainShader->setInt("u_irradianceMap", 10);
        
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_iblTextures.prefilterMap);
        m_terrainShader->setInt("u_prefilterMap", 11);
        
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_2D, m_iblTextures.brdfLUT);
        m_terrainShader->setInt("u_brdfLUT", 12);
    }
    
    // Set transformation matrices
    glm::mat4 projection = glm::perspective(
        glm::radians(m_camera->getZoom()), 
        (float)m_viewportWidth / (float)m_viewportHeight, 
        0.1f, 300.0f
    );
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);
    
    m_terrainShader->setMat4("projection", projection);
    m_terrainShader->setMat4("view", view);
    m_terrainShader->setMat4("model", model);
    
    // Render skybox
    if (m_iblTextures.isValid()) {
        glDepthFunc(GL_LEQUAL);
        m_skyboxShader->use();
        m_skyboxShader->setMat4("view", view);
        m_skyboxShader->setMat4("projection", projection);
        m_skyboxShader->setFloat("exposure", 1.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_iblTextures.environmentMap);
        m_skyboxShader->setInt("skybox", 0);
        
        HDRLoader::renderUnitCube();
        
        glDepthFunc(GL_LESS);
    }
    
    // Render terrain
    m_terrainShader->use();
    m_terrainMesh->draw(m_terrainShader);
}

