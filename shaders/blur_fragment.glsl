#version 330 core

// Gaussian blur fragment shader - applies separable blur for bloom effect
// Uses ping-pong technique with horizontal/vertical passes for performance

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_image;      // Image to blur
uniform bool u_horizontal;      // True for horizontal pass, false for vertical

// Gaussian blur weights for 9-tap kernel
// Pre-calculated for sigma = 2.0
uniform float weights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(u_image, 0); // Size of single texel
    vec3 result = texture(u_image, TexCoord).rgb * weights[0]; // Current fragment
    
    if (u_horizontal) {
        // Horizontal blur pass
        for (int i = 1; i < 5; ++i) {
            // Sample in both directions for symmetric blur
            result += texture(u_image, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
            result += texture(u_image, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
        }
    } else {
        // Vertical blur pass
        for (int i = 1; i < 5; ++i) {
            // Sample in both directions for symmetric blur
            result += texture(u_image, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weights[i];
            result += texture(u_image, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weights[i];
        }
    }
    
    FragColor = vec4(result, 1.0);
}
