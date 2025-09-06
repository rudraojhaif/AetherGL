#version 330 core

// Debug version of final post-processing shader
// Just passes through the scene texture to isolate rendering issues

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_sceneTexture;    // Main scene color

void main() {
    // Simple pass-through - just sample the scene texture
    vec3 color = texture(u_sceneTexture, TexCoord).rgb;
    
    // Output the color directly without any effects
    FragColor = vec4(color, 1.0);
}
