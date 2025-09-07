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

in vec3 FragPos;     // World position
in vec2 TexCoord;    // Screen coordinates [0,1]

// Atmospheric scattering uniforms
uniform vec3 u_sunDirection;       // Normalized sun direction
uniform vec3 u_sunColor;          // Sun color and intensity
uniform float u_sunIntensity;     // Sun brightness multiplier
uniform vec3 u_cameraPos;         // Camera world position
uniform float u_time;             // Time for animation

// Atmospheric parameters
uniform float u_atmosphereRadius;    // Atmosphere outer radius
uniform float u_planetRadius;       // Planet inner radius
uniform float u_rayleighCoeff;      // Rayleigh scattering coefficient
uniform float u_mieCoeff;           // Mie scattering coefficient
uniform float u_mieG;               // Mie phase function asymmetry
uniform float u_sunDiskSize;        // Size of sun disk
uniform float u_exposure;           // HDR exposure adjustment

// Fog parameters
uniform float u_fogDensity;         // Base fog density
uniform float u_fogHeightFalloff;   // Height-based fog falloff
uniform vec3 u_fogColor;           // Base fog color

const float PI = 3.14159265359;
const int ATMOSPHERE_SAMPLES = 16;  // Quality vs performance
const int LIGHT_SAMPLES = 8;       // Light ray marching samples

/**
 * Atmospheric Scattering Implementation
 * Based on Sean O'Neil's "Accurate Atmospheric Scattering" with optimizations
 */

// Phase function for Rayleigh scattering
float rayleighPhase(float cosTheta) {
    return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
}

// Phase function for Mie scattering (Henyey-Greenstein)
float miePhase(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

// Exponential atmosphere density
float atmosphereDensity(float altitude, float scaleHeight) {
    return exp(-altitude / scaleHeight);
}

// Ray-sphere intersection
bool raySphereIntersect(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius, out float t1, out float t2) {
    vec3 oc = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(oc, rayDir);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4.0 * a * c;
    
    if (discriminant < 0.0) {
        return false;
    }
    
    float sqrtD = sqrt(discriminant);
    t1 = (-b - sqrtD) / (2.0 * a);
    t2 = (-b + sqrtD) / (2.0 * a);
    return true;
}

// Calculate optical depth along a ray
float calculateOpticalDepth(vec3 rayStart, vec3 rayDir, float rayLength, float scaleHeight) {
    float stepSize = rayLength / float(LIGHT_SAMPLES);
    float opticalDepth = 0.0;
    
    for (int i = 0; i < LIGHT_SAMPLES; i++) {
        vec3 samplePoint = rayStart + rayDir * (float(i) + 0.5) * stepSize;
        float altitude = length(samplePoint) - u_planetRadius;
        float density = atmosphereDensity(altitude, scaleHeight);
        opticalDepth += density * stepSize;
    }
    
    return opticalDepth;
}

// Main atmospheric scattering calculation
vec3 calculateAtmosphericScattering(vec3 rayOrigin, vec3 rayDir, vec3 sunDir) {
    // Ray-atmosphere intersection
    float t1, t2;
    vec3 planetCenter = vec3(0.0, -u_planetRadius, 0.0);
    
    if (!raySphereIntersect(rayOrigin, rayDir, planetCenter, u_atmosphereRadius, t1, t2)) {
        return vec3(0.0); // Ray misses atmosphere
    }
    
    // Ensure we start from atmosphere entry point
    float rayStart = max(t1, 0.0);
    float rayEnd = t2;
    float rayLength = rayEnd - rayStart;
    
    if (rayLength <= 0.0) return vec3(0.0);
    
    // Ray marching through atmosphere
    float stepSize = rayLength / float(ATMOSPHERE_SAMPLES);
    vec3 totalRayleigh = vec3(0.0);
    vec3 totalMie = vec3(0.0);
    
    float rayleighOpticalDepth = 0.0;
    float mieOpticalDepth = 0.0;
    
    for (int i = 0; i < ATMOSPHERE_SAMPLES; i++) {
        vec3 samplePoint = rayOrigin + rayDir * (rayStart + (float(i) + 0.5) * stepSize);
        float altitude = length(samplePoint) - u_planetRadius;
        
        // Atmosphere density at sample point
        float rayleighDensity = atmosphereDensity(altitude, 8000.0); // 8km scale height
        float mieDensity = atmosphereDensity(altitude, 1200.0);      // 1.2km scale height
        
        rayleighOpticalDepth += rayleighDensity * stepSize;
        mieOpticalDepth += mieDensity * stepSize;
        
        // Calculate light optical depth (sun to sample point)
        float sunT1, sunT2;
        if (raySphereIntersect(samplePoint, sunDir, planetCenter, u_atmosphereRadius, sunT1, sunT2)) {
            float sunRayLength = sunT2;
            float sunRayleighOpticalDepth = calculateOpticalDepth(samplePoint, sunDir, sunRayLength, 8000.0);
            float sunMieOpticalDepth = calculateOpticalDepth(samplePoint, sunDir, sunRayLength, 1200.0);
            
            // Calculate transmittance
            vec3 rayleighTransmittance = exp(-u_rayleighCoeff * vec3(5.8e-6, 13.5e-6, 33.1e-6) * (rayleighOpticalDepth + sunRayleighOpticalDepth));
            vec3 mieTransmittance = exp(-u_mieCoeff * (mieOpticalDepth + sunMieOpticalDepth));
            
            // Accumulate in-scattered light
            totalRayleigh += rayleighTransmittance * rayleighDensity * stepSize;
            totalMie += mieTransmittance * mieDensity * stepSize;
        }
    }
    
    // Apply phase functions
    float cosTheta = dot(rayDir, sunDir);
    float rayleighPhaseValue = rayleighPhase(cosTheta);
    float miePhaseValue = miePhase(cosTheta, u_mieG);
    
    // Combine scattering
    vec3 rayleighColor = totalRayleigh * u_rayleighCoeff * vec3(5.8e-6, 13.5e-6, 33.1e-6) * rayleighPhaseValue;
    vec3 mieColor = totalMie * u_mieCoeff * miePhaseValue * vec3(2.0e-5);
    
    return (rayleighColor + mieColor) * u_sunIntensity * u_sunColor;
}

// Calculate sun disk
vec3 calculateSunDisk(vec3 rayDir, vec3 sunDir) {
    float sunDot = dot(rayDir, sunDir);
    float sunDisk = smoothstep(cos(u_sunDiskSize), 1.0, sunDot);
    
    // Add corona effect
    float corona = pow(max(0.0, sunDot), 256.0) * 0.5;
    
    return (sunDisk + corona) * u_sunColor * u_sunIntensity;
}

// Exponential height fog
vec3 applyVolumetricFog(vec3 worldPos, vec3 cameraPos, vec3 color) {
    vec3 rayDir = worldPos - cameraPos;
    float distance = length(rayDir);
    rayDir /= distance;
    
    // Sample fog density along ray
    int fogSamples = 8;
    float stepSize = distance / float(fogSamples);
    float totalFog = 0.0;
    
    for (int i = 0; i < fogSamples; i++) {
        vec3 samplePos = cameraPos + rayDir * (float(i) + 0.5) * stepSize;
        float height = samplePos.y;
        
        // Exponential height fog
        float fogDensity = u_fogDensity * exp(-height * u_fogHeightFalloff);
        totalFog += fogDensity * stepSize;
    }
    
    float fogFactor = exp(-totalFog);
    
    // Atmospheric perspective - fog gets sun color in sun direction
    float sunAlignment = max(0.0, dot(rayDir, -u_sunDirection));
    vec3 fogColorAdjusted = mix(u_fogColor, u_sunColor * 0.5, sunAlignment * 0.3);
    
    return mix(fogColorAdjusted, color, fogFactor);
}

void main() {
    // Convert screen coordinates to world ray
    vec3 rayOrigin = u_cameraPos;
    
    // Calculate ray direction from screen coordinates
    vec2 screenPos = TexCoord * 2.0 - 1.0; // Convert to [-1,1]
    
    // Simple ray direction calculation (assumes looking along negative Z)
    // In a full implementation, you'd use inverse view-projection matrix
    vec3 rayDir = normalize(vec3(screenPos.x, screenPos.y, -1.0));
    
    // Calculate atmospheric scattering
    vec3 scatteredColor = calculateAtmosphericScattering(rayOrigin, rayDir, -u_sunDirection);
    
    // Add sun disk
    vec3 sunDisk = calculateSunDisk(rayDir, -u_sunDirection);
    
    // Combine sky color
    vec3 skyColor = scatteredColor + sunDisk;
    
    // Apply volumetric fog if ray hits terrain (simplified)
    if (rayDir.y < 0.0) {
        // Approximate terrain intersection
        float terrainDistance = -rayOrigin.y / rayDir.y;
        vec3 terrainPos = rayOrigin + rayDir * terrainDistance;
        skyColor = applyVolumetricFog(terrainPos, rayOrigin, skyColor);
    }
    
    // HDR tone mapping
    skyColor *= u_exposure;
    skyColor = skyColor / (skyColor + vec3(1.0)); // Reinhard tone mapping
    
    // Gamma correction
    skyColor = pow(skyColor, vec3(1.0/2.2));
    
    FragColor = vec4(skyColor, 1.0);
}
