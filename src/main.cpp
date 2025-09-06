#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "TerrainGenerator.h"

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

    // Load terrain shaders
    auto terrainShader = std::make_shared<Shader>("terrain_vertex.glsl", "terrain_fragment.glsl");

    // Generate procedural terrain mesh
    // Using medium quality settings for balanced performance and visual quality
    std::shared_ptr<Mesh> terrainMesh = TerrainGenerator::generateTerrainMesh(
        50.0f,  // Width: 50 world units
        50.0f,  // Depth: 50 world units  
        128,    // Width segments: higher resolution for detailed terrain
        128,    // Depth segments: higher resolution for detailed terrain
        glm::vec3(0.0f, -2.0f, 0.0f)  // Center position (slightly below camera)
    );
    
    if (!terrainMesh) {
        std::cerr << "Failed to generate terrain mesh!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "Procedural terrain generated successfully!" << std::endl;

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

        // Set lighting uniforms
        terrainShader->setVec3("lightPos", glm::vec3(20.0f, 15.0f, 20.0f));  // Higher light for terrain
        terrainShader->setVec3("lightColor", glm::vec3(1.0f, 0.95f, 0.8f)); // Warm sunlight
        terrainShader->setVec3("viewPos", camera->getPosition());

        // Set terrain generation parameters
        terrainShader->setFloat("u_time", static_cast<float>(glfwGetTime()));
        terrainShader->setFloat("u_scale", 0.1f);         // Terrain noise scale
        terrainShader->setFloat("u_heightScale", 3.0f);   // Terrain height multiplier
        terrainShader->setInt("u_octaves", 6);            // Number of noise octaves
        terrainShader->setFloat("u_persistence", 0.5f);   // Amplitude persistence
        terrainShader->setFloat("u_lacunarity", 2.0f);    // Frequency lacunarity

        // Set transformation matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera->getZoom()), 
                                              (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                              0.1f, 200.0f);  // Increased far plane for terrain
        glm::mat4 view = camera->getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        
        // No rotation needed for terrain - keep it stationary
        terrainShader->setMat4("projection", projection);
        terrainShader->setMat4("view", view);
        terrainShader->setMat4("model", model);

        // Draw terrain mesh
        terrainMesh->draw(terrainShader);
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
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