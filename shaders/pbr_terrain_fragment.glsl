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

MaterialProperties getTerrainMaterial(float worldHeight, vec3 normal, vec2 detailTexCoords, int materialType, float surfaceDetail) {
    MaterialProperties mat;
    
    // Calculate slope
    float slope = 1.0 - abs(dot(normalize(normal), vec3(0.0, 1.0, 0.0)));
    
    // Base materials with PBR properties
    vec3 grassAlbedo = vec3(0.2, 0.7, 0.1);
    vec3 rockAlbedo = vec3(0.4, 0.35, 0.3);
    vec3 snowAlbedo = vec3(0.95, 0.97, 1.0);
    vec3 dirtAlbedo = vec3(0.3, 0.25, 0.2);
    
    // Initialize with dirt
    mat.albedo = dirtAlbedo;
    mat.metallic = 0.0;
    mat.roughness = 0.9;
    
    // Grass areas
    if (worldHeight > u_grassHeight && slope < 0.4) {
        float grassBlend = smoothstep(u_grassHeight, u_grassHeight + 1.0, worldHeight) *
                          (1.0 - smoothstep(0.2, 0.6, slope));
        mat.albedo = mix(mat.albedo, grassAlbedo * mix(0.8, 1.2, surfaceDetail), grassBlend);
        mat.roughness = mix(mat.roughness, 0.8, grassBlend);
        mat.metallic = mix(mat.metallic, 0.0, grassBlend);
    }
    
    // Rock areas
    if (worldHeight > u_rockHeight || slope > 0.3) {
        float rockBlend = max(
            smoothstep(u_rockHeight, u_rockHeight + 2.0, worldHeight),
            smoothstep(0.3, 0.7, slope)
        );
        mat.albedo = mix(mat.albedo, rockAlbedo * mix(0.7, 1.1, surfaceDetail), rockBlend);
        mat.roughness = mix(mat.roughness, 0.7, rockBlend);
        mat.metallic = mix(mat.metallic, 0.1, rockBlend);
    }
    
    // Snow areas
    if (worldHeight > u_snowHeight && slope < 0.5) {
        float snowBlend = smoothstep(u_snowHeight, u_snowHeight + 1.5, worldHeight) *
                         (1.0 - smoothstep(0.3, 0.7, slope));
        mat.albedo = mix(mat.albedo, snowAlbedo * mix(0.95, 1.0, surfaceDetail), snowBlend);
        mat.roughness = mix(mat.roughness, 0.1, snowBlend);
        mat.metallic = mix(mat.metallic, 0.0, snowBlend);
    }
    
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
    
    // Atmospheric perspective
    float distance = length(viewPos - FragPos);
    float fog = exp(-distance * 0.005);
    vec3 fogColor = vec3(0.7, 0.8, 0.9);
    color = mix(fogColor, color, fog);
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
