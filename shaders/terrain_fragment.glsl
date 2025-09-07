// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is the property of Rudra Ojha.  
// Redistribution, modification, or use of this code in any project  
// (commercial or non-commercial) is strictly prohibited without  
// prior written consent from the author.
//
// Unauthorized usage will be considered a violation of copyright law.

#version 330 core

out vec4 FragColor;

in vec3 FragPos;     // World position from vertex shader
in vec3 Normal;      // Surface normal from vertex shader
in vec2 TexCoord;    // Texture coordinates

// Lighting uniforms
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Terrain material uniforms
uniform float u_grassHeight;    // Height threshold for grass
uniform float u_rockHeight;     // Height threshold for rock
uniform float u_snowHeight;     // Height threshold for snow

// Parallax Occlusion Mapping uniforms
uniform float u_pomScale;       // POM depth scale
uniform int u_pomMinSamples;    // Minimum samples for POM
uniform int u_pomMaxSamples;    // Maximum samples for POM
uniform bool u_enablePOM;       // Enable/disable POM

/**
 * Terrain Fragment Shader with Parallax Occlusion Mapping
 * 
 * This shader combines:
 * 1. CPU-calculated vertex displacement for large-scale terrain elevation
 * 2. GPU-based Parallax Occlusion Mapping for fine surface detail
 * 
 * This dual approach provides both realistic terrain elevation (like Minecraft)
 * and detailed surface textures without excessive geometry complexity.
 */

/**
 * Procedural heightmap generation for POM
 * Generates detailed height information for surface relief
 */
float generateDetailHeight(vec2 uv, int materialType) {
    // Use high-frequency noise for surface detail
    vec2 p = uv * 32.0; // High frequency for surface detail
    
    // Simple fractal noise for heightmap
    float height = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    
    for (int i = 0; i < 4; i++) {
        // Simple pseudo-random noise (replace with proper noise if needed)
        vec2 pos = p * frequency;
        float h = fract(sin(dot(pos, vec2(12.9898, 78.233))) * 43758.5453);
        h = h * 2.0 - 1.0; // Convert to [-1, 1]
        
        height += h * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    // Material-specific height adjustments
    if (materialType == 0) { // Grass - softer, less pronounced
        height *= 0.3;
    } else if (materialType == 1) { // Rock - sharp, pronounced
        height = abs(height) * 0.8; // Sharp ridges
    } else if (materialType == 2) { // Snow - smooth, subtle
        height *= 0.2;
    }
    
    return clamp(height * 0.5 + 0.5, 0.0, 1.0); // Normalize to [0, 1]
}

/**
 * Parallax Occlusion Mapping implementation
 * Creates illusion of surface depth and detail
 */
vec2 parallaxOcclusionMapping(vec2 texCoords, vec3 viewDir, int materialType) {
    if (!u_enablePOM) {
        return texCoords;
    }
    
    // Number of depth layers based on viewing angle
    float numLayers = mix(float(u_pomMaxSamples), float(u_pomMinSamples), 
                         abs(dot(vec3(0.0, 1.0, 0.0), viewDir)));
    
    // Calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    
    // The amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * u_pomScale;
    vec2 deltaTexCoords = P / numLayers;
    
    // Initial values
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = generateDetailHeight(currentTexCoords, materialType);
    
    // Linear search
    while (currentLayerDepth < currentDepthMapValue) {
        // Shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // Get depthmap value at current texture coordinates
        currentDepthMapValue = generateDetailHeight(currentTexCoords, materialType);
        // Get depth of next layer
        currentLayerDepth += layerDepth;
    }
    
    // Binary search for more precision (optional, can be commented out for performance)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = generateDetailHeight(prevTexCoords, materialType) - 
                       (currentLayerDepth - layerDepth);
    
    // Interpolation
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords;
}

/**
 * Determine the dominant material type for POM
 */
int getDominantMaterialType(float worldHeight, float slope) {
    // 0 = Grass, 1 = Rock, 2 = Snow
    
    // Snow at high elevations
    if (worldHeight > u_snowHeight && slope < 0.5) {
        return 2; // Snow
    }
    
    // Rock on steep slopes or high elevations
    if (worldHeight > u_rockHeight || slope > 0.3) {
        return 1; // Rock
    }
    
    // Grass on gentle slopes at moderate elevations
    if (worldHeight > u_grassHeight && slope < 0.4) {
        return 0; // Grass
    }
    
    return 1; // Default to rock/dirt
}

/**
 * Enhanced material selection with POM-based surface detail
 */
vec3 getTerrainMaterial(float worldHeight, vec3 normal, vec2 detailTexCoords, int materialType) {
    // Define realistic base material colors
    vec3 grassColor = vec3(0.2, 0.7, 0.1);      // Vibrant green grass  
    vec3 rockColor = vec3(0.3, 0.2, 0.1);       // Dark brown rock
    vec3 snowColor = vec3(1.0, 1.0, 1.0);       // Pure bright white snow
    vec3 dirtColor = vec3(0.4, 0.3, 0.2);       // Dark brown dirt
    
    // Calculate slope factor (steeper slopes = more rock exposure)
    float slope = 1.0 - abs(dot(normalize(normal), vec3(0.0, 1.0, 0.0)));
    
    // Get surface detail from heightmap for color variation
    float surfaceDetail = generateDetailHeight(detailTexCoords, materialType);
    
    // Start with base dirt color
    vec3 finalColor = dirtColor;
    
    // Grass grows on gentle slopes at low to medium elevations
    if (worldHeight > u_grassHeight && slope < 0.4) {
        float grassBlend = smoothstep(u_grassHeight, u_grassHeight + 1.0, worldHeight) *
                          (1.0 - smoothstep(0.2, 0.6, slope));
        vec3 grassVariation = grassColor * mix(0.7, 1.3, surfaceDetail); // Color variation
        finalColor = mix(finalColor, grassVariation, grassBlend);
    }
    
    // Rock appears on steep slopes and higher elevations
    if (worldHeight > u_rockHeight || slope > 0.3) {
        float rockBlend = max(
            smoothstep(u_rockHeight, u_rockHeight + 2.0, worldHeight),
            smoothstep(0.3, 0.7, slope)
        );
        vec3 rockVariation = rockColor * mix(0.6, 1.2, surfaceDetail); // Color variation
        finalColor = mix(finalColor, rockVariation, rockBlend * 0.8);
    }
    
    // Snow caps appear at high elevations on gentle slopes
    if (worldHeight > u_snowHeight && slope < 0.5) {
        float snowBlend = smoothstep(u_snowHeight, u_snowHeight + 1.5, worldHeight) *
                         (1.0 - smoothstep(0.3, 0.7, slope));
        vec3 snowVariation = snowColor * mix(0.9, 1.0, surfaceDetail); // Subtle variation
        finalColor = mix(finalColor, snowVariation, snowBlend);
    }
    
    return finalColor;
}

void main() {
    // Use the actual world height from vertex displacement
    float worldHeight = FragPos.y;
    
    // Use the interpolated normal from vertex shader (calculated from displaced mesh)
    vec3 surfaceNormal = normalize(Normal);
    
    // Calculate slope for material determination
    float slope = 1.0 - abs(dot(surfaceNormal, vec3(0.0, 1.0, 0.0)));
    
    // Determine the dominant material type for POM
    int materialType = getDominantMaterialType(worldHeight, slope);
    
    // === PARALLAX OCCLUSION MAPPING ===
    vec2 detailTexCoords = TexCoord;
    if (u_enablePOM) {
        // Calculate view direction in tangent space for POM
        vec3 viewDir = normalize(viewPos - FragPos);
        
        // Apply parallax occlusion mapping
        detailTexCoords = parallaxOcclusionMapping(TexCoord, viewDir, materialType);
        
        // Calculate enhanced normal from heightmap for better lighting
        float epsilon = 0.01;
        float heightL = generateDetailHeight(detailTexCoords - vec2(epsilon, 0.0), materialType);
        float heightR = generateDetailHeight(detailTexCoords + vec2(epsilon, 0.0), materialType);
        float heightU = generateDetailHeight(detailTexCoords - vec2(0.0, epsilon), materialType);
        float heightD = generateDetailHeight(detailTexCoords + vec2(0.0, epsilon), materialType);
        
        // Calculate detail normal
        vec3 detailNormal = normalize(vec3(
            (heightL - heightR) * u_pomScale * 50.0,
            (heightU - heightD) * u_pomScale * 50.0,
            1.0
        ));
        
        // Blend detail normal with surface normal
        // Simple normal blending - could be enhanced with proper tangent space calculations
        surfaceNormal = normalize(surfaceNormal + detailNormal * 0.3);
    }
    
    // Get enhanced material color with surface detail
    vec3 materialColor = getTerrainMaterial(worldHeight, surfaceNormal, detailTexCoords, materialType);
    
    // === LIGHTING CALCULATIONS ===
    
    // Ambient lighting
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(surfaceNormal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting (material-dependent)
    float specularStrength = 0.05;
    if (materialType == 1) { // Rock - more reflective
        specularStrength = 0.1;
    } else if (materialType == 2) { // Snow - highly reflective
        specularStrength = 0.3;
    }
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, surfaceNormal);
    float shininess = materialType == 2 ? 32.0 : 8.0; // Snow is shinier
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Apply atmospheric perspective for depth
    float distance = length(viewPos - FragPos);
    float fog = exp(-distance * 0.008); // Subtle fog effect
    vec3 fogColor = vec3(0.7, 0.8, 0.9); // Light blue atmospheric color
    
    // Combine all lighting components
    vec3 result = (ambient + diffuse + specular) * materialColor;
    
    // Apply fog for atmospheric depth
    result = mix(fogColor, result, fog);
    
    // Output final color
    FragColor = vec4(result, 1.0);
}
