#version 330 core

// Final post-processing fragment shader
// Combines bloom, depth of field, chromatic aberration, tone mapping, and gamma correction

in vec2 TexCoord;
out vec4 FragColor;

// Input textures
uniform sampler2D u_sceneTexture;    // Main scene color
uniform sampler2D u_depthTexture;    // Scene depth buffer
uniform sampler2D u_bloomTexture;    // Bloom blur result

// Effect toggles
uniform bool u_enableBloom;
uniform bool u_enableDOF;
uniform bool u_enableChromaticAberration;

// Bloom settings
uniform float u_bloomIntensity;

// Depth of Field settings
uniform float u_focusDistance;       // Camera focus distance
uniform float u_focusRange;          // Range around focus that stays sharp
uniform float u_bokehRadius;         // Maximum blur radius

// Chromatic Aberration settings
uniform float u_aberrationStrength;  // Aberration displacement amount

// Tone mapping and gamma
uniform float u_exposure;
uniform float u_gamma;

// Convert linear depth to world space distance (approximation)
float linearizeDepth(float depth) {
    float near = 0.1;  // Near plane
    float far = 300.0; // Far plane (match main.cpp projection)
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

// Simple bokeh-style depth of field blur
vec3 applyDepthOfField(vec2 uv, float depth) {
    float distance = linearizeDepth(depth);
    
    // Calculate blur amount based on distance from focus
    float blur = abs(distance - u_focusDistance);
    blur = smoothstep(u_focusRange, u_focusRange + 5.0, blur) * u_bokehRadius;
    
    if (blur < 0.5) {
        // No blur needed, return original sample
        return texture(u_sceneTexture, uv).rgb;
    }
    
    // Apply bokeh blur with circular sampling pattern
    vec3 color = vec3(0.0);
    float totalWeight = 0.0;
    
    // Simple 8-sample circular pattern for bokeh effect
    int samples = 8;
    float angleStep = 6.28318 / float(samples); // 2π / samples
    
    for (int i = 0; i < samples; i++) {
        float angle = float(i) * angleStep;
        vec2 offset = vec2(cos(angle), sin(angle)) * blur * 0.01; // Scale down offset
        
        vec3 sample = texture(u_sceneTexture, uv + offset).rgb;
        color += sample;
        totalWeight += 1.0;
    }
    
    // Add center sample with higher weight
    color += texture(u_sceneTexture, uv).rgb * 2.0;
    totalWeight += 2.0;
    
    return color / totalWeight;
}

// Chromatic aberration effect
vec3 applyChromaticAberration(vec2 uv) {
    // Calculate distance from center
    vec2 center = vec2(0.5, 0.5);
    vec2 dir = uv - center;
    float distance = length(dir) * 2.0; // Scale for stronger effect at edges
    
    // Calculate aberration offset
    vec2 aberration = normalize(dir) * distance * u_aberrationStrength * 0.01;
    
    // Sample each color channel with slight offset
    float r = texture(u_sceneTexture, uv + aberration).r;
    float g = texture(u_sceneTexture, uv).g;
    float b = texture(u_sceneTexture, uv - aberration).b;
    
    return vec3(r, g, b);
}

// ACES tone mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    // Start with the original scene color
    vec3 color = texture(u_sceneTexture, TexCoord).rgb;
    
    // Apply effects in sequence
    
    // 1. Apply chromatic aberration if enabled
    if (u_enableChromaticAberration) {
        color = applyChromaticAberration(TexCoord);
    }
    
    // 2. Apply depth of field if enabled (after chromatic aberration)
    if (u_enableDOF) {
        float depth = texture(u_depthTexture, TexCoord).r;
        color = applyDepthOfField(TexCoord, depth);
    }
    
    // 3. Add bloom if enabled
    if (u_enableBloom) {
        vec3 bloomColor = texture(u_bloomTexture, TexCoord).rgb;
        color += bloomColor * u_bloomIntensity;
    }
    
    // 4. Apply exposure (only if we have HDR content or effects enabled)
    if (u_enableBloom || u_enableDOF || u_enableChromaticAberration) {
        color *= u_exposure;
        
        // 5. Tone mapping (ACES) - only when effects are enabled
        color = ACESFilm(color);
    } else {
        // For non-HDR content without effects, apply minimal exposure
        color *= min(u_exposure, 1.0);
    }
    
    // 6. Gamma correction
    color = pow(color, vec3(1.0 / u_gamma));
    
    FragColor = vec4(color, 1.0);
}
