#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <string>

#include "TerrainRenderer.h"

// Global variables
std::unique_ptr<TerrainRenderer> g_terrainRenderer;
bool g_firstMouse = true;
float g_lastX = 400.0f;
float g_lastY = 300.0f;
bool g_captureMouse = false;

// Configuration functions
void createDefaultConfigFile() {
    std::ofstream configFile("settings.txt");
    if (configFile.is_open()) {
        configFile << "# AetherGL Configuration File\n";
        configFile << "# Edit these values and restart the application to apply changes\n\n";
        
        configFile << "[Lighting]\n";
        configFile << "sun_intensity=3.0\n";
        configFile << "sun_color_r=1.0\n";
        configFile << "sun_color_g=0.95\n";
        configFile << "sun_color_b=0.8\n";
        configFile << "sun_direction_x=-0.3\n";
        configFile << "sun_direction_y=-1.0\n";
        configFile << "sun_direction_z=-0.2\n";
        configFile << "fog_density=0.02\n";
        configFile << "fog_height=50.0\n";
        configFile << "fog_color_r=0.7\n";
        configFile << "fog_color_g=0.8\n";
        configFile << "fog_color_b=0.9\n\n";
        
        configFile << "[Post-Processing]\n";
        configFile << "enable_post_processing=true\n";
        configFile << "enable_bloom=true\n";
        configFile << "bloom_threshold=1.0\n";
        configFile << "bloom_intensity=0.8\n";
        configFile << "bloom_iterations=5\n";
        configFile << "enable_dof=false\n";
        configFile << "focus_distance=50.0\n";
        configFile << "dof_strength=1.0\n";
        configFile << "enable_chromatic_aberration=true\n";
        configFile << "chromatic_aberration_strength=0.5\n";
        configFile << "exposure=1.0\n\n";
        
        configFile << "[Camera]\n";
        configFile << "position_x=0.0\n";
        configFile << "position_y=50.0\n";
        configFile << "position_z=100.0\n";
        configFile << "movement_speed=50.0\n";
        configFile << "mouse_sensitivity=0.1\n\n";
        
        configFile.close();
        std::cout << "Created default configuration file: settings.txt" << std::endl;
    }
}

void loadConfiguration() {
    std::ifstream configFile("settings.txt");
    if (!configFile.is_open()) {
        std::cout << "Configuration file not found, creating default settings.txt" << std::endl;
        createDefaultConfigFile();
        return;
    }
    
    if (!g_terrainRenderer) {
        std::cout << "Warning: TerrainRenderer not initialized yet" << std::endl;
        return;
    }
    
    std::string line;
    std::string currentSection = "";
    
    auto& lighting = g_terrainRenderer->getLightingConfig();
    auto ppConfig = g_terrainRenderer->getPostProcessorConfig();
    
    std::cout << "Loading configuration from settings.txt..." << std::endl;
    
    while (std::getline(configFile, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        // Check for section headers
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                currentSection = line.substr(1, end - 1);
            }
            continue;
        }
        
        // Parse key=value pairs
        size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        
        std::string key = line.substr(0, equals);
        std::string value = line.substr(equals + 1);
        
        try {
            if (currentSection == "Lighting") {
                if (key == "sun_intensity") lighting.directionalLight.intensity = std::stof(value);
                else if (key == "sun_color_r") lighting.directionalLight.color.r = std::stof(value);
                else if (key == "sun_color_g") lighting.directionalLight.color.g = std::stof(value);
                else if (key == "sun_color_b") lighting.directionalLight.color.b = std::stof(value);
                else if (key == "sun_direction_x") lighting.directionalLight.direction.x = std::stof(value);
                else if (key == "sun_direction_y") lighting.directionalLight.direction.y = std::stof(value);
                else if (key == "sun_direction_z") lighting.directionalLight.direction.z = std::stof(value);
                else if (key == "fog_density") lighting.atmosphere.fogDensity = std::stof(value);
                else if (key == "fog_height") lighting.atmosphere.fogHeightFalloff = std::stof(value);
                else if (key == "fog_color_r") lighting.atmosphere.fogColor.r = std::stof(value);
                else if (key == "fog_color_g") lighting.atmosphere.fogColor.g = std::stof(value);
                else if (key == "fog_color_b") lighting.atmosphere.fogColor.b = std::stof(value);
            }
            else if (currentSection == "Post-Processing") {
                if (key == "enable_post_processing") {
                    bool enabled = (value == "true" || value == "1");
                    g_terrainRenderer->setPostProcessingEnabled(enabled);
                }
                else if (key == "enable_bloom") ppConfig.enableBloom = (value == "true" || value == "1");
                else if (key == "bloom_threshold") ppConfig.bloomThreshold = std::stof(value);
                else if (key == "bloom_intensity") ppConfig.bloomIntensity = std::stof(value);
                else if (key == "bloom_iterations") ppConfig.bloomBlurPasses = std::stoi(value);
                else if (key == "enable_dof") ppConfig.enableDOF = (value == "true" || value == "1");
                else if (key == "focus_distance") ppConfig.focusDistance = std::stof(value);
                else if (key == "dof_strength") ppConfig.bokehRadius = std::stof(value);
                else if (key == "enable_chromatic_aberration") ppConfig.enableChromaticAberration = (value == "true" || value == "1");
                else if (key == "chromatic_aberration_strength") ppConfig.aberrationStrength = std::stof(value);
                else if (key == "exposure") ppConfig.exposure = std::stof(value);
            }
        } catch (const std::exception& e) {
            std::cout << "Warning: Error parsing config value '" << key << "' = '" << value << "': " << e.what() << std::endl;
        }
    }
    
    // Apply the loaded post-processing config
    g_terrainRenderer->setPostProcessorConfig(ppConfig);
    
    configFile.close();
    std::cout << "Configuration loaded successfully!" << std::endl;
}

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Ignore invalid dimensions
    if (width <= 0 || height <= 0) {
        std::cout << "Ignoring invalid framebuffer size: " << width << "x" << height << std::endl;
        return;
    }
    
    // Clear any pending OpenGL errors before resize
    while (glGetError() != GL_NO_ERROR);
    
    std::cout << "Framebuffer resize callback: " << width << "x" << height << std::endl;
    
    glViewport(0, 0, width, height);
    
    if (g_terrainRenderer) {
        try {
            g_terrainRenderer->resize(width, height);
        } catch (const std::exception& e) {
            std::cerr << "Error during terrain renderer resize: " << e.what() << std::endl;
        }
    }
    
    // Check for errors after resize
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error in framebuffer resize callback: " << error << std::endl;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS && !g_captureMouse) {
        g_firstMouse = true;
        return;
    }

    if (g_firstMouse) {
        g_lastX = static_cast<float>(xpos);
        g_lastY = static_cast<float>(ypos);
        g_firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - g_lastX);
    float yoffset = static_cast<float>(g_lastY - ypos);

    g_lastX = static_cast<float>(xpos);
    g_lastY = static_cast<float>(ypos);

    if (g_terrainRenderer) {
        g_terrainRenderer->processMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_terrainRenderer) {
        g_terrainRenderer->processMouseScroll(static_cast<float>(yoffset));
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        return;
    }
    
    // Toggle mouse capture with Tab
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        g_captureMouse = !g_captureMouse;
        if (g_captureMouse) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            g_firstMouse = true;
        }
        return;
    }
    
    // Reload configuration with R
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        std::cout << "\nReloading configuration..." << std::endl;
        loadConfiguration();
        return;
    }

    if (g_terrainRenderer) {
        g_terrainRenderer->processKeyboard(key, action);
    }
}


int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    // Create window
    GLFWwindow* window = glfwCreateWindow(1600, 1200, "AetherGL - Procedural Terrain Generator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }


    // Print OpenGL info
    std::cout << "\n=== AetherGL Terrain Generator ===\n" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "- Left mouse + drag: Look around (or use Tab for mouse capture)" << std::endl;
    std::cout << "- WASD: Move camera" << std::endl;
    std::cout << "- Mouse wheel: Zoom in/out" << std::endl;
    std::cout << "- Tab: Toggle mouse capture" << std::endl;
    std::cout << "- ESC: Exit application" << std::endl;
    std::cout << "- R: Reload configuration from settings.txt" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "- Edit 'settings.txt' to customize lighting and post-processing" << std::endl;
    std::cout << "- Restart the application or press 'R' to reload settings" << std::endl;
    std::cout << "\n" << std::endl;

    // Create terrain renderer
    try {
        g_terrainRenderer = std::make_unique<TerrainRenderer>();
        
        // Get initial window size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        g_terrainRenderer->initialize(width, height);
        
        std::cout << "Terrain renderer initialized successfully!" << std::endl;
        
        // Load configuration from file
        loadConfiguration();
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize terrain renderer: " << e.what() << std::endl;
        
        
        glfwTerminate();
        return -1;
    }

    // Enable V-Sync
    glfwSwapInterval(1);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Process events
        glfwPollEvents();

        // Render terrain
        if (g_terrainRenderer) {
            g_terrainRenderer->render();
        }
        

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    g_terrainRenderer.reset();
    
    
    glfwTerminate();

    std::cout << "Application closed successfully!" << std::endl;
    return 0;
}
