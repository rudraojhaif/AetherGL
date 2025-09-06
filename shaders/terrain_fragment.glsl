#version 330 core

out vec4 FragColor;

in vec3 FragPos;     // World position from vertex shader
in vec3 Normal;      // Surface normal
in vec2 TexCoord;    // Texture coordinates (optional)

// Lighting uniforms
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Terrain generation uniforms
uniform float u_time;           // Animation time
uniform float u_scale;          // Overall terrain scale
uniform float u_heightScale;    // Height multiplier
uniform int u_octaves;          // Number of noise octaves
uniform float u_persistence;    // Amplitude persistence
uniform float u_lacunarity;     // Frequency lacunarity

/**
 * 3D Simplex noise implementation
 * Based on Stefan Gustavson's implementation
 */
vec3 mod289(vec3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
    return mod289(((x * 34.0) + 1.0) * x);
}

vec4 taylorInvSqrt(vec4 r) {
    return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    // Permutations
    i = mod289(i);
    vec4 p = permute(permute(permute(
                i.z + vec4(0.0, i1.z, i2.z, 1.0))
            + i.y + vec4(0.0, i1.y, i2.y, 1.0))
        + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    // Gradients: 7x7 points over a square, mapped onto an octahedron
    float n_ = 0.142857142857; // 1.0/7.0
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z); // mod(p,7*7)

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_); // mod(j,N)

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    // Normalize gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

/**
 * Fractal Brownian Motion (fBm) noise
 * Combines multiple octaves of noise with decreasing amplitude and increasing frequency
 */
float fbm(vec3 pos, int octaves, float persistence, float lacunarity) {
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        value += snoise(pos * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return value / maxValue; // Normalize to [-1, 1]
}

/**
 * Generate terrain height at a given world position
 */
float getTerrainHeight(vec3 worldPos) {
    vec3 noisePos = worldPos * u_scale;
    
    // Add time-based animation for dynamic terrain (optional)
    noisePos.y += u_time * 0.1;
    
    // Generate base terrain using fBm
    float height = fbm(noisePos, u_octaves, u_persistence, u_lacunarity);
    
    // Apply height scaling and bias
    height = height * u_heightScale + 0.1;
    
    return height;
}

/**
 * Calculate terrain normal using finite differences
 * This provides proper lighting for the procedural terrain
 */
vec3 calculateTerrainNormal(vec3 worldPos, float epsilon) {
    float heightCenter = getTerrainHeight(worldPos);
    float heightRight = getTerrainHeight(worldPos + vec3(epsilon, 0.0, 0.0));
    float heightUp = getTerrainHeight(worldPos + vec3(0.0, 0.0, epsilon));
    
    vec3 tangentRight = vec3(epsilon, heightRight - heightCenter, 0.0);
    vec3 tangentUp = vec3(0.0, heightUp - heightCenter, epsilon);
    
    return normalize(cross(tangentRight, tangentUp));
}

/**
 * Procedural material blending based on terrain height and slope
 */
vec3 getTerrainMaterial(float height, vec3 normal) {
    // Define material colors
    vec3 rockColor = vec3(0.4, 0.3, 0.25);      // Dark brown rock
    vec3 grassColor = vec3(0.2, 0.6, 0.1);      // Green grass
    vec3 snowColor = vec3(0.95, 0.95, 0.95);    // White snow
    vec3 dirtColor = vec3(0.5, 0.35, 0.2);      // Brown dirt/soil
    
    // Normalize height to [0, 1] range for material selection
    float normalizedHeight = clamp((height + 1.0) * 0.5, 0.0, 1.0);
    
    // Calculate slope factor (steeper slopes = more rock)
    float slope = 1.0 - dot(normal, vec3(0.0, 1.0, 0.0));
    
    vec3 finalColor = dirtColor; // Base color
    
    // Grass layer (low to mid elevations, gentle slopes)
    float grassMask = smoothstep(0.1, 0.4, normalizedHeight) * 
                     (1.0 - smoothstep(0.6, 0.8, normalizedHeight)) *
                     (1.0 - smoothstep(0.3, 0.7, slope));
    finalColor = mix(finalColor, grassColor, grassMask);
    
    // Rock layer (steep slopes or mid elevations)
    float rockMask = smoothstep(0.2, 0.6, slope) + 
                    smoothstep(0.4, 0.7, normalizedHeight) * (1.0 - smoothstep(0.8, 1.0, normalizedHeight));
    rockMask = clamp(rockMask, 0.0, 1.0);
    finalColor = mix(finalColor, rockColor, rockMask);
    
    // Snow layer (high elevations)
    float snowMask = smoothstep(0.7, 0.9, normalizedHeight) * 
                    (1.0 - smoothstep(0.5, 1.0, slope));
    finalColor = mix(finalColor, snowColor, snowMask);
    
    return finalColor;
}

void main() {
    // Calculate terrain height and normal at current fragment
    float terrainHeight = getTerrainHeight(FragPos);
    vec3 terrainNormal = calculateTerrainNormal(FragPos, 0.01);
    
    // Get procedural material color based on height and slope
    vec3 materialColor = getTerrainMaterial(terrainHeight, terrainNormal);
    
    // === LIGHTING CALCULATIONS ===
    
    // Ambient lighting
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(terrainNormal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting (reduced for terrain realism)
    float specularStrength = 0.1;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, terrainNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0); // Lower shininess for terrain
    vec3 specular = specularStrength * spec * lightColor;
    
    // Apply atmospheric perspective (distance-based color shift)
    float distance = length(viewPos - FragPos);
    float fog = exp(-distance * 0.01); // Exponential fog
    vec3 fogColor = vec3(0.7, 0.8, 0.9); // Light blue atmospheric color
    
    // Combine lighting with material
    vec3 result = (ambient + diffuse + specular) * materialColor;
    
    // Apply fog
    result = mix(fogColor, result, fog);
    
    // Output final color
    FragColor = vec4(result, 1.0);
}
