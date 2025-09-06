/**
 * Shader.cpp - OpenGL shader program management
 * 
 * Handles the complete shader pipeline:
 * - Loading GLSL source code from files
 * - Compiling vertex and fragment shaders  
 * - Linking shaders into a complete program
 * - Providing uniform variable setters
 * - Error checking and reporting
 * 
 * OpenGL Shader Pipeline:
 * 1. Vertex Shader: Processes individual vertices
 * 2. Fragment Shader: Processes individual pixels/fragments
 * 3. Program: Links shaders together for GPU execution
 */

#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>  // For glm::value_ptr()
#include <fstream>               // File input/output
#include <sstream>               // String stream operations
#include <iostream>              // Console output for error reporting

/**
 * Shader constructor - loads, compiles, and links vertex and fragment shaders
 * 
 * OpenGL shader compilation process:
 * 1. Create shader objects with glCreateShader()
 * 2. Load source code with glShaderSource() 
 * 3. Compile with glCompileShader()
 * 4. Create program with glCreateProgram()
 * 5. Attach shaders with glAttachShader()
 * 6. Link program with glLinkProgram()
 * 7. Clean up individual shader objects
 * 
 * @param vertexPath File path to vertex shader source code
 * @param fragmentPath File path to fragment shader source code
 */
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    // Load shader source code from files
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);
    
    // Convert to C-style strings for OpenGL API
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    // Compile vertex shader
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);        // Create vertex shader object
    glShaderSource(vertex, 1, &vShaderCode, NULL);                // Set source code
    glCompileShader(vertex);                                       // Compile shader
    checkCompileErrors(vertex, "VERTEX");                          // Check for compilation errors
    
    // Compile fragment shader
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);    // Create fragment shader object
    glShaderSource(fragment, 1, &fShaderCode, NULL);              // Set source code
    glCompileShader(fragment);                                     // Compile shader
    checkCompileErrors(fragment, "FRAGMENT");                      // Check for compilation errors
    
    // Create and link shader program
    m_program = glCreateProgram();                                 // Create program object
    glAttachShader(m_program, vertex);                            // Attach vertex shader
    glAttachShader(m_program, fragment);                          // Attach fragment shader
    glLinkProgram(m_program);                                     // Link shaders into program
    checkCompileErrors(m_program, "PROGRAM");                     // Check for linking errors
    
    // Clean up individual shader objects (no longer needed after linking)
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

/**
 * Shader destructor - releases GPU resources
 * 
 * Properly cleans up OpenGL program object to prevent memory leaks
 * This is called automatically when Shader object goes out of scope
 */
Shader::~Shader() {
    glDeleteProgram(m_program);  // Release GPU program memory
}

/**
 * Activate this shader program for rendering
 * 
 * Tells OpenGL to use this shader program for subsequent draw calls
 * Only one shader program can be active at a time in OpenGL
 */
void Shader::use() {
    glUseProgram(m_program);  // Set as active shader program
}

/**
 * Read shader source code from file
 * 
 * Loads the entire file contents into a string for shader compilation
 * Uses stringstream for efficient string building from file buffer
 * 
 * @param filepath Path to the shader source file
 * @return Complete file contents as string, empty on error
 */
std::string Shader::readFile(const std::string& filepath) {
    std::ifstream file;          // Input file stream
    std::stringstream stream;    // String stream for efficient concatenation
    
    // Configure file stream to throw exceptions on read errors
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        file.open(filepath);        // Open the shader file
        stream << file.rdbuf();     // Read entire file buffer into stream
        file.close();               // Close file handle
        return stream.str();        // Convert stream to string
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filepath << std::endl;
        return "";  // Return empty string on failure
    }
}

/**
 * Check for shader compilation and program linking errors
 * 
 * OpenGL error checking is essential because:
 * - Shader compilation happens on GPU, errors need explicit checking
 * - Failed shaders will cause rendering to fail silently or crash
 * - Error messages help debug GLSL syntax and linking issues
 * 
 * @param shader OpenGL shader/program object ID
 * @param type Type of object being checked ("VERTEX", "FRAGMENT", "PROGRAM")
 */
void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success;           // Compilation/linking success status
    char infoLog[1024];    // Buffer for error messages
    
    if (type != "PROGRAM") {
        // Check individual shader compilation
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
        }
    }
    else {
        // Check shader program linking
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
        }
    }
}

/**
 * Set boolean uniform variable in shader
 * 
 * Uniform variables are global GLSL variables that remain constant
 * for all vertices/fragments in a single draw call. Used for:
 * - Lighting parameters, transformation matrices, material properties
 * 
 * @param name GLSL uniform variable name
 * @param value Boolean value to set (converted to integer)
 */
void Shader::setBool(const std::string& name, bool value) const {
    GLint location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {  // SAFETY: Check if uniform exists before setting
        glUniform1i(location, (int)value);
    }
}

/**
 * Set integer uniform variable in shader
 * 
 * @param name GLSL uniform variable name  
 * @param value Integer value to set
 */
void Shader::setInt(const std::string& name, int value) const {
    GLint location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {  // SAFETY: Check if uniform exists before setting
        glUniform1i(location, value);
    }
}

/**
 * Set float uniform variable in shader
 * 
 * @param name GLSL uniform variable name
 * @param value Float value to set
 */
void Shader::setFloat(const std::string& name, float value) const {
    GLint location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {  // SAFETY: Check if uniform exists before setting
        glUniform1f(location, value);
    }
}

/**
 * Set 3D vector uniform variable in shader
 * 
 * Commonly used for positions, colors, directions, and other 3D data
 * glm::value_ptr() converts GLM vector to raw float pointer for OpenGL
 * 
 * @param name GLSL uniform variable name
 * @param value GLM 3D vector to set
 */
void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    GLint location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {  // SAFETY: Check if uniform exists before setting
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

/**
 * Set 4x4 matrix uniform variable in shader
 * 
 * Essential for 3D transformations:
 * - Model matrix: object-to-world transformation
 * - View matrix: world-to-camera transformation  
 * - Projection matrix: camera-to-screen transformation
 * 
 * GL_FALSE indicates matrix is in column-major order (GLM default)
 * 
 * @param name GLSL uniform variable name
 * @param mat GLM 4x4 matrix to set
 */
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    GLint location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {  // SAFETY: Check if uniform exists before setting
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
    }
}