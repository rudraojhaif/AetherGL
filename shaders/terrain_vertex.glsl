#version 330 core

// Input vertex attributes
layout (location = 0) in vec3 aPos;       // Vertex position
layout (location = 1) in vec3 aNormal;    // Vertex normal
layout (location = 2) in vec2 aTexCoord;  // Texture coordinates

// Output to fragment shader
out vec3 FragPos;    // World space position for noise sampling
out vec3 Normal;     // World space normal for lighting
out vec2 TexCoord;   // Texture coordinates

// Transformation matrices
uniform mat4 model;      // Model matrix (object to world transform)
uniform mat4 view;       // View matrix (world to camera transform)  
uniform mat4 projection; // Projection matrix (camera to clip space transform)

/**
 * Terrain Vertex Shader
 * 
 * This vertex shader is optimized for procedural terrain rendering.
 * It passes the world position to the fragment shader where the actual
 * terrain height calculation and material blending occurs using fBm noise.
 * 
 * Key features:
 * - Proper normal transformation for lighting calculations
 * - World position output for noise sampling in fragment shader
 * - Standard vertex transformation pipeline
 * - Efficient uniform usage following industry practices
 */
void main() {
    // Transform vertex position to world space
    // This is essential for the fragment shader to sample noise correctly
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Transform normal to world space
    // Use the normal matrix (inverse transpose of model matrix) to properly
    // handle non-uniform scaling and maintain perpendicularity
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Pass through texture coordinates unchanged
    // These will be used for any texture-based effects in the fragment shader
    TexCoord = aTexCoord;
    
    // Standard vertex transformation pipeline:
    // Model -> View -> Projection
    // This transforms the vertex from local space to clip space
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
