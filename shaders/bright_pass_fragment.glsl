#version 330 core

// Bright pass fragment shader - extracts bright areas for bloom effect
// Filters out pixels below a brightness threshold

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_sceneTexture;   // Main scene texture
uniform float u_threshold;          // Brightness threshold for bloom

void main() {
    vec3 color = texture(u_sceneTexture, TexCoord).rgb;
    
    // Calculate brightness using luminance formula
    // Using ITU-R BT.709 luma coefficients for accurate brightness perception
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Extract bright areas above threshold
    if (brightness > u_threshold) {
        // Preserve bright pixels, with smooth falloff near threshold
        float excess = brightness - u_threshold;
        FragColor = vec4(color * excess, 1.0);
    } else {
        // Discard dark pixels
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
