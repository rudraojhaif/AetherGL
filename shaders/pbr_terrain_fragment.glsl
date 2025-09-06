#version 330 core

out vec4 FragColor;

in vec3 FragPos;     // World position from vertex shader
in vec3 Normal;      // Surface normal from vertex shader
in vec2 TexCoord;    // Texture coordinates

// PBR Lighting uniforms
uniform vec3 viewPos;

// Directional Light
uniform bool u_enableDirLight;
uniform vec3 u_dirLightDir;
uniform vec3 u_dirLightColor;
uniform float u_dirLightIntensity;

// Point Lights (maximum 8)
uniform int u_numPointLights;
uniform vec3 u_pointLightPositions[8];
uniform vec3 u_pointLightColors[8];
uniform float u_pointLightIntensities[8];
uniform float u_pointLightRanges[8];

// IBL uniforms
uniform bool u_enableIBL;
uniform samplerCube u_irradianceMap;
uniform samplerCube u_prefilterMap;
uniform sampler2D u_brdfLUT;
uniform float u_iblIntensity;

// Atmospheric & Volumetric Fog uniforms
uniform bool u_enableAtmosphericFog;
uniform float u_fogDensity;
uniform float u_fogHeightFalloff;
uniform vec3 u_fogColor;
uniform float u_atmosphericPerspective;
uniform vec3 u_sunDirection;

// Terrain material uniforms
uniform float u_grassHeight;
uniform float u_rockHeight;
uniform float u_snowHeight;

// Enhanced POM uniforms
uniform bool u_enablePOM;
uniform float u_pomScale;
uniform int u_pomMinSamples;
uniform int u_pomMaxSamples;
uniform float u_pomSharpen;

const float PI = 3.14159265359;

/**
 * Enhanced procedural heightmap with reduced noise and better quality
 */
float generateDetailHeight(vec2 uv, int materialType) {
    vec2 p = uv;
    float height = 0.0;
    
    // Multi-octave noise with better frequency distribution
    for (int i = 0; i < 6; i++) {
        vec2 pos = p * pow(2.0, float(i) * 1.2) * 16.0;
        
        // Improved noise function with smoother characteristics
        float h1 = fract(sin(dot(pos, vec2(127.1, 311.7))) * 43758.5453);
        float h2 = fract(sin(dot(pos + vec2(1.0, 0.0), vec2(127.1, 311.7))) * 43758.5453);
        float h3 = fract(sin(dot(pos + vec2(0.0, 1.0), vec2(127.1, 311.7))) * 43758.5453);
        float h4 = fract(sin(dot(pos + vec2(1.0, 1.0), vec2(127.1, 311.7))) * 43758.5453);
        
        // Bilinear interpolation for smoother result
        vec2 f = fract(pos);
        f = f * f * (3.0 - 2.0 * f); // Smoothstep interpolation
        
        float h12 = mix(h1, h2, f.x);
        float h34 = mix(h3, h4, f.x);
        float h = mix(h12, h34, f.y);
        
        h = h * 2.0 - 1.0; // Convert to [-1, 1]
        height += h * pow(0.5, float(i));
    }
    
    // Material-specific adjustments
    if (materialType == 0) { // Grass - organic, soft patterns
        height = height * 0.4 + sin(uv.x * 50.0) * sin(uv.y * 50.0) * 0.1;
    } else if (materialType == 1) { // Rock - sharp, angular features
        height = abs(height) * 0.7;
        height += max(0.0, sin(uv.x * 80.0 + uv.y * 80.0)) * 0.3;
    } else if (materialType == 2) { // Snow - smooth, windswept
        height = height * 0.25;
        height = smoothstep(-0.3, 0.3, height);
    }
    
    return clamp(height * 0.5 + 0.5, 0.0, 1.0);
}

/**
 * High-Quality Parallax Occlusion Mapping with reduced artifacts
 */
vec2 parallaxOcclusionMapping(vec2 texCoords, vec3 viewDir, int materialType) {
    if (!u_enablePOM) {
        return texCoords;
    }
    
    // Adaptive sample count based on viewing angle
    float viewDotNormal = abs(dot(vec3(0.0, 1.0, 0.0), viewDir));
    float numSamples = mix(float(u_pomMaxSamples), float(u_pomMinSamples), viewDotNormal);
    
    // Layer depth
    float layerDepth = 1.0 / numSamples;
    float currentLayerDepth = 0.0;
    
    // Offset per layer
    vec2 P = viewDir.xy / viewDir.z * u_pomScale;
    vec2 deltaTexCoords = P / numSamples;
    
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = generateDetailHeight(currentTexCoords, materialType);
    
    // Linear search
    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = generateDetailHeight(currentTexCoords, materialType);
        currentLayerDepth += layerDepth;
    }
    
    // Enhanced binary search for higher precision (reduces granularity)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    
    // Multiple binary search iterations for smoother results
    for (int i = 0; i < 5; i++) {
        vec2 midTexCoords = (currentTexCoords + prevTexCoords) * 0.5;
        float midDepth = generateDetailHeight(midTexCoords, materialType);
        float midLayerDepth = currentLayerDepth - layerDepth * 0.5;
        
        if (midDepth > midLayerDepth) {
            currentTexCoords = midTexCoords;
            currentDepthMapValue = midDepth;
        } else {
            prevTexCoords = midTexCoords;
        }
        layerDepth *= 0.5;
    }
    
    // Final interpolation for even smoother result
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = generateDetailHeight(prevTexCoords, materialType) - (currentLayerDepth - layerDepth);
    
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    // Clamp to prevent texture coordinate overflow
    finalTexCoords = clamp(finalTexCoords, texCoords - P, texCoords + P);
    
    return finalTexCoords;
}

/**
 * GGX/Trowbridge-Reitz Normal Distribution Function
 */
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

/**
 * Smith's Geometric Attenuation Function
 */
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

/**
 * Fresnel-Schlick Approximation
 */
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/**
 * Fresnel-Schlick with roughness for IBL
 */
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/**
 * Enhanced material properties based on terrain type
 */
struct MaterialProperties {
    vec3 albedo;
    float metallic;
    float roughness;
    vec3 F0;
};

/**
 * Enhanced material system with clearer biome differentiation
 */
MaterialProperties getTerrainMaterial(float worldHeight, vec3 normal, vec2 detailTexCoords, int materialType, float surfaceDetail) {
    MaterialProperties mat;
    
    // Calculate slope factor (0 = flat, 1 = vertical)
    float slope = 1.0 - abs(dot(normalize(normal), vec3(0.0, 1.0, 0.0)));
    
    // Enhanced material colors with more contrast
    vec3 grassAlbedo = vec3(0.15, 0.8, 0.05);    // Vibrant green
    vec3 rockAlbedo = vec3(0.6, 0.5, 0.4);       // Light brown rock
    vec3 snowAlbedo = vec3(0.98, 0.99, 1.0);     // Pure white snow
    vec3 dirtAlbedo = vec3(0.25, 0.2, 0.15);     // Dark brown dirt
    
    // Determine dominant biome with sharper transitions
    float grassFactor = 0.0;
    float rockFactor = 0.0;
    float snowFactor = 0.0;
    float dirtFactor = 1.0; // Base layer
    
    // GRASS BIOME - Lower elevations, gentle slopes
    if (worldHeight >= u_grassHeight - 0.5f) {
        float heightFactor = smoothstep(u_grassHeight - 0.5f, u_grassHeight + 1.0f, worldHeight);
        float slopeFactor = 1.0 - smoothstep(0.15, 0.4, slope); // Gentle slopes only
        grassFactor = heightFactor * slopeFactor;
        
        // Reduce grass at very high elevations
        if (worldHeight > u_rockHeight) {
            grassFactor *= 1.0 - smoothstep(u_rockHeight, u_rockHeight + 2.0, worldHeight);
        }
    }
    
    // ROCK BIOME - Steep slopes or higher elevations
    if (slope > 0.2 || worldHeight > u_rockHeight - 1.0f) {
        float slopeFactor = smoothstep(0.2, 0.6, slope);  // Steep slope rocks
        float heightFactor = smoothstep(u_rockHeight - 1.0f, u_rockHeight + 1.0f, worldHeight); // High elevation rocks
        rockFactor = max(slopeFactor, heightFactor * 0.8);
    }
    
    // SNOW BIOME - Highest elevations, not too steep
    if (worldHeight >= u_snowHeight - 1.0f) {
        float heightFactor = smoothstep(u_snowHeight - 1.0f, u_snowHeight + 0.5f, worldHeight);
        float slopeFactor = 1.0 - smoothstep(0.3, 0.8, slope); // Snow doesn't stick on steep slopes
        snowFactor = heightFactor * slopeFactor;
    }
    
    // Normalize factors so they add up to 1.0 for proper blending
    float totalFactor = dirtFactor + grassFactor + rockFactor + snowFactor;
    if (totalFactor > 0.0) {
        dirtFactor /= totalFactor;
        grassFactor /= totalFactor;
        rockFactor /= totalFactor;
        snowFactor /= totalFactor;
    }
    
    // Apply sharper blending with surface detail variation
    vec3 finalAlbedo = vec3(0.0);
    float finalRoughness = 0.0;
    float finalMetallic = 0.0;
    
    // Dirt base
    finalAlbedo += dirtAlbedo * mix(0.8, 1.1, surfaceDetail) * dirtFactor;
    finalRoughness += 0.95 * dirtFactor;
    finalMetallic += 0.0 * dirtFactor;
    
    // Grass areas with more variation
    if (grassFactor > 0.0) {
        vec3 grassVariation = grassAlbedo * mix(0.7, 1.3, surfaceDetail);
        // Add some brown/yellow tinting for realism
        grassVariation += vec3(0.1, 0.05, 0.0) * surfaceDetail;
        finalAlbedo += grassVariation * grassFactor;
        finalRoughness += 0.85 * grassFactor;
        finalMetallic += 0.0 * grassFactor;
    }
    
    // Rock areas with mineral variation
    if (rockFactor > 0.0) {
        vec3 rockVariation = rockAlbedo * mix(0.6, 1.2, surfaceDetail);
        // Add some reddish/gray tinting
        rockVariation += vec3(0.05, 0.02, 0.0) * (surfaceDetail - 0.5) * 2.0;
        finalAlbedo += rockVariation * rockFactor;
        finalRoughness += 0.75 * rockFactor;
        finalMetallic += 0.15 * rockFactor; // Slightly metallic for minerals
    }
    
    // Snow areas
    if (snowFactor > 0.0) {
        vec3 snowVariation = snowAlbedo * mix(0.95, 1.0, surfaceDetail * 0.5);
        finalAlbedo += snowVariation * snowFactor;
        finalRoughness += 0.05 * snowFactor; // Very smooth
        finalMetallic += 0.0 * snowFactor;
    }
    
    // Assign final material properties
    mat.albedo = finalAlbedo;
    mat.roughness = clamp(finalRoughness, 0.05, 0.95);
    mat.metallic = clamp(finalMetallic, 0.0, 1.0);
    
    // Calculate F0 (base reflectance)
    mat.F0 = mix(vec3(0.04), mat.albedo, mat.metallic);
    
    return mat;
}

/**
 * Calculate lighting contribution from a point light
 */
vec3 calculatePointLight(vec3 lightPos, vec3 lightColor, float lightIntensity, float lightRange,
                        vec3 fragPos, vec3 N, vec3 V, MaterialProperties material) {
    vec3 L = normalize(lightPos - fragPos);
    vec3 H = normalize(V + L);
    
    // Calculate attenuation
    float distance = length(lightPos - fragPos);
    float attenuation = clamp(1.0 - (distance / lightRange), 0.0, 1.0);
    attenuation = attenuation * attenuation;
    vec3 radiance = lightColor * lightIntensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, material.roughness);
    float G = geometrySmith(N, V, L, material.roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), material.F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * material.albedo / PI + specular) * radiance * NdotL;
}

/**
 * Calculate lighting contribution from directional light
 */
vec3 calculateDirectionalLight(vec3 lightDir, vec3 lightColor, float lightIntensity,
                              vec3 N, vec3 V, MaterialProperties material) {
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    
    vec3 radiance = lightColor * lightIntensity;
    
    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, material.roughness);
    float G = geometrySmith(N, V, L, material.roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), material.F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * material.albedo / PI + specular) * radiance * NdotL;
}

/**
 * Advanced Volumetric Fog with Atmospheric Perspective
 */
vec3 calculateVolumetricFog(vec3 worldPos, vec3 cameraPos, vec3 color) {
    if (!u_enableAtmosphericFog) {
        return color;
    }
    
    vec3 viewVector = worldPos - cameraPos;
    float distance = length(viewVector);
    vec3 rayDir = viewVector / distance;
    
    // Enhanced exponential height fog
    float avgHeight = (worldPos.y + cameraPos.y) * 0.5;
    float heightFactor = exp(-avgHeight * u_fogHeightFalloff);
    
    // Distance-based fog density
    float fogAmount = 1.0 - exp(-distance * u_fogDensity * heightFactor);
    
    // Atmospheric perspective - objects get atmospheric color
    float sunAlignment = max(0.0, dot(rayDir, normalize(-u_sunDirection)));
    
    // Fog color varies based on sun alignment and height
    vec3 sunScatter = mix(vec3(0.6, 0.7, 0.8), vec3(1.0, 0.8, 0.6), sunAlignment);
    vec3 atmosphericColor = mix(u_fogColor, sunScatter, u_atmosphericPerspective * sunAlignment);
    
    // Height-based color variation (cooler at higher altitudes)
    float heightColorMix = clamp(avgHeight / 20.0, 0.0, 1.0);
    atmosphericColor = mix(atmosphericColor, vec3(0.5, 0.6, 0.8), heightColorMix * 0.3);
    
    return mix(color, atmosphericColor, fogAmount);
}

/**
 * Calculate atmospheric light scattering for enhanced realism
 */
vec3 calculateAtmosphericLighting(vec3 worldPos, vec3 cameraPos, vec3 lightDir, vec3 lightColor) {
    vec3 viewDir = normalize(worldPos - cameraPos);
    float distance = length(worldPos - cameraPos);
    
    // Simple atmospheric scattering approximation
    float scatterAmount = 1.0 - exp(-distance * 0.01);
    float sunAlignment = max(0.0, dot(viewDir, normalize(-lightDir)));
    
    // Rayleigh scattering (blue sky)
    vec3 rayleighColor = vec3(0.3, 0.6, 1.0) * pow(sunAlignment, 2.0);
    
    // Mie scattering (sun halo)
    float mieStrength = pow(sunAlignment, 8.0) * 0.5;
    vec3 mieColor = lightColor * mieStrength;
    
    return (rayleighColor + mieColor) * scatterAmount * 0.1;
}

void main() {
    // Get world height and surface normal
    float worldHeight = FragPos.y;
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // Calculate slope for material determination
    float slope = 1.0 - abs(dot(N, vec3(0.0, 1.0, 0.0)));
    
    // Determine dominant material type
    int materialType = 1; // Default to rock/dirt
    if (worldHeight > u_snowHeight && slope < 0.5) {
        materialType = 2; // Snow
    } else if (worldHeight > u_grassHeight && slope < 0.4) {
        materialType = 0; // Grass
    }
    
    // Apply Parallax Occlusion Mapping
    vec2 detailTexCoords = TexCoord;
    vec3 enhancedNormal = N;
    
    if (u_enablePOM) {
        detailTexCoords = parallaxOcclusionMapping(TexCoord, V, materialType);
        
        // Calculate enhanced normal from heightmap
        float epsilon = 1.0 / 2048.0; // Higher precision
        float heightL = generateDetailHeight(detailTexCoords - vec2(epsilon, 0.0), materialType);
        float heightR = generateDetailHeight(detailTexCoords + vec2(epsilon, 0.0), materialType);
        float heightU = generateDetailHeight(detailTexCoords - vec2(0.0, epsilon), materialType);
        float heightD = generateDetailHeight(detailTexCoords + vec2(0.0, epsilon), materialType);
        
        vec3 detailNormal = normalize(vec3(
            (heightL - heightR) * u_pomScale * 100.0,
            (heightU - heightD) * u_pomScale * 100.0,
            1.0
        ));
        
        enhancedNormal = normalize(N + detailNormal * 0.5);
    }
    
    // Get surface detail and material properties
    float surfaceDetail = generateDetailHeight(detailTexCoords, materialType);
    MaterialProperties material = getTerrainMaterial(worldHeight, enhancedNormal, detailTexCoords, materialType, surfaceDetail);
    
    vec3 color = vec3(0.0);
    
    // Directional Light
    if (u_enableDirLight) {
        color += calculateDirectionalLight(u_dirLightDir, u_dirLightColor, u_dirLightIntensity, enhancedNormal, V, material);
    }
    
    // Point Lights
    for (int i = 0; i < u_numPointLights && i < 8; ++i) {
        color += calculatePointLight(u_pointLightPositions[i], u_pointLightColors[i], u_pointLightIntensities[i], u_pointLightRanges[i], FragPos, enhancedNormal, V, material);
    }
    
    // IBL Ambient Lighting (simplified for now - full IBL would require precomputed maps)
    if (u_enableIBL) {
        vec3 F = fresnelSchlickRoughness(max(dot(enhancedNormal, V), 0.0), material.F0, material.roughness);
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - material.metallic;
        
        vec3 irradiance = texture(u_irradianceMap, enhancedNormal).rgb;
        vec3 diffuse = irradiance * material.albedo;
        
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 R = reflect(-V, enhancedNormal);
        vec3 prefilteredColor = textureLod(u_prefilterMap, R, material.roughness * MAX_REFLECTION_LOD).rgb;
        
        vec2 brdf = texture(u_brdfLUT, vec2(max(dot(enhancedNormal, V), 0.0), material.roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        
        vec3 ambient = (kD * diffuse + specular) * u_iblIntensity;
        color += ambient;
    } else {
        // Simple ambient fallback
        color += material.albedo * 0.03;
    }
    
    // Enhanced atmospheric and volumetric effects
    vec3 atmosphericLighting = calculateAtmosphericLighting(FragPos, viewPos, u_dirLightDir, u_dirLightColor);
    color += atmosphericLighting;
    
    // Apply volumetric fog with atmospheric perspective
    color = calculateVolumetricFog(FragPos, viewPos, color);
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
