#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <iomanip>

#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "TerrainGenerator.h"
#include "LightingConfig.h"
#include "HDRLoader.h"

// Settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Global variables
std::unique_ptr<Camera> camera;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Procedural Terrain Generator", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Initialize camera with better starting position for terrain viewing
    camera = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 10.0f));

    // Load PBR terrain shaders
    auto terrainShader = std::make_shared<Shader>("terrain_vertex.glsl", "pbr_terrain_fragment.glsl");
    
    // Load skybox shader
    auto skyboxShader = std::make_shared<Shader>("skybox_vertex.glsl", "skybox_fragment.glsl");

    // Generate procedural terrain mesh with vertex displacement
    // Using medium quality settings for balanced performance and visual quality
    std::shared_ptr<Mesh> terrainMesh = TerrainGenerator::generateTerrainMesh(
        80.0f,  // Width: 80 world units
        80.0f,  // Depth: 80 world units  
        100,    // Width segments: good balance of detail and performance
        100,    // Depth segments: good balance of detail and performance
        glm::vec3(0.0f, 0.0f, 0.0f),  // Center position
        15.0f,  // Height scale: maximum terrain elevation
        0.08f,  // Noise scale: controls terrain "zoom" level
        42      // Seed: for reproducible terrain (change for different terrain)
    );
    
    if (!terrainMesh) {
        std::cerr << "Failed to generate terrain mesh!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "Procedural terrain generated successfully!" << std::endl;
    
    // Load HDR environment
    std::cout << "Loading HDR environment..." << std::endl;
    HDRLoader::IBLTextures iblTextures = HDRLoader::loadHDREnvironment("assets/qwantani_noon_puresky_4k.hdr");
    
    if (!iblTextures.isValid()) {
        std::cerr << "Warning: Failed to load HDR environment. IBL will be disabled." << std::endl;
    } else {
        std::cout << "HDR environment loaded successfully!" << std::endl;
    }

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use terrain shader
        terrainShader->use();

        // === EASY LIGHTING CONFIGURATION ===
        // You can easily change lighting here!
        
        static LightingConfig lighting; // Create once
        static bool lightingInitialized = false;
        
        if (!lightingInitialized) {
            // Choose your lighting setup:
            lighting.setupDefaultLights();      // Daytime with sun + accent lights
            // lighting.setupSunsetLighting();   // Dramatic sunset scene
            // lighting.setupNightLighting();    // Night scene with multiple colorful lights
            
            // === ATMOSPHERIC & FOG CONFIGURATION ===
            lighting.atmosphere.enableAtmosphericFog = true;
            lighting.atmosphere.fogDensity = 0.015f;              // Subtle fog
            lighting.atmosphere.fogHeightFalloff = 0.08f;         // Height-based falloff
            lighting.atmosphere.atmosphericPerspective = 0.7f;    // Strong atmospheric perspective
            
            // === HDR/IBL CONFIGURATION ===
            lighting.ibl.enabled = iblTextures.isValid();         // Enable IBL if HDR loaded
            lighting.ibl.intensity = 0.8f;                       // Strong IBL contribution
            
            // === TIME-OF-DAY SYSTEM ===
            lighting.timeOfDay.animateTimeOfDay = true;           // Enable day/night cycle
            lighting.timeOfDay.daySpeed = 0.05f;                 // Speed of cycle (slow)
            lighting.timeOfDay.timeOfDay = 0.3f;                 // Start at morning
            
            // === ADVANCED CUSTOMIZATION ===
            // Uncomment to manually control:
            // lighting.atmosphere.fogColor = glm::vec3(0.8f, 0.6f, 0.4f); // Warm fog
            // lighting.atmosphere.fogDensity = 0.03f;            // Thicker fog
            // lighting.timeOfDay.animateTimeOfDay = false;       // Static time
            // lighting.directionalLight.direction = glm::vec3(0.3f, -0.6f, 0.1f);
            
            // Enhanced POM for atmospheric scenes
            lighting.pom.scale = 0.1f;         // Good balance
            lighting.pom.maxSamples = 48;      // High quality
            
            // === BIOME HEIGHT ADJUSTMENTS ===
            // Adjust these values for clearer biome separation
            lighting.terrain.grassHeight = 1.0f;   // Grass starts lower (more visible)
            lighting.terrain.rockHeight = 6.0f;    // Rock starts at mid elevation
            lighting.terrain.snowHeight = 10.0f;   // Snow caps start earlier
            
            lightingInitialized = true;
            std::cout << "Atmospheric terrain system with HDR IBL initialized!" << std::endl;
        }
        
        // Update time-of-day animation
        lighting.updateTimeOfDay(deltaTime);
        
        // Display atmospheric info (optional debug output)
        static float infoTimer = 0.0f;
        infoTimer += deltaTime;
        if (infoTimer > 5.0f) { // Every 5 seconds
            if (lighting.timeOfDay.animateTimeOfDay) {
                float timeHours = lighting.timeOfDay.timeOfDay * 24.0f;
                int hours = (int)timeHours;
                int minutes = (int)((timeHours - hours) * 60.0f);
                std::cout << "Time: " << std::setfill('0') << std::setw(2) << hours 
                         << ":" << std::setfill('0') << std::setw(2) << minutes
                         << " | Sun Intensity: " << lighting.directionalLight.intensity << std::endl;
            }
            infoTimer = 0.0f;
        }
        
        // Apply all lighting settings to shader
        lighting.applyToShader(terrainShader, camera->getPosition());
        
        // Bind IBL textures if available
        if (iblTextures.isValid()) {
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblTextures.irradianceMap);
            terrainShader->setInt("u_irradianceMap", 10);
            
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblTextures.prefilterMap);
            terrainShader->setInt("u_prefilterMap", 11);
            
            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D, iblTextures.brdfLUT);
            terrainShader->setInt("u_brdfLUT", 12);
        }

        // Set transformation matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera->getZoom()), 
                                              (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                              0.1f, 300.0f);  // Large far plane for expansive terrain
        glm::mat4 view = camera->getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        
        // No rotation needed for terrain - keep it stationary
        terrainShader->setMat4("projection", projection);
        terrainShader->setMat4("view", view);
        terrainShader->setMat4("model", model);

        // === RENDER SKYBOX ===
        if (iblTextures.isValid()) {
            glDepthFunc(GL_LEQUAL); // Change depth function for skybox
            skyboxShader->use();
            skyboxShader->setMat4("view", view);
            skyboxShader->setMat4("projection", projection);
            skyboxShader->setFloat("exposure", 1.0f);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblTextures.environmentMap);
            skyboxShader->setInt("skybox", 0);
            
            HDRLoader::renderUnitCube(); // Render skybox cube
            
            glDepthFunc(GL_LESS); // Reset depth function
        }
        
        // === RENDER TERRAIN ===
        terrainShader->use(); // Make sure terrain shader is active
        
        // Draw terrain mesh
        terrainMesh->draw(terrainShader);
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup HDR resources
    if (iblTextures.isValid()) {
        HDRLoader::cleanup(iblTextures);
    }
    
    // Cleanup
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera->processKeyboard(CameraMovement::DOWN, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera->processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera->processMouseScroll(static_cast<float>(yoffset));
}