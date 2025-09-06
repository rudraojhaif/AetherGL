#include "NoiseGenerator.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <numeric>

NoiseGenerator::NoiseGenerator(unsigned int seed) : m_permutation(512) {
    setSeed(seed);
}

void NoiseGenerator::setSeed(unsigned int seed) {
    if (seed == 0) {
        // Use current time as seed if none provided
        seed = static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    
    m_rng.seed(seed);
    initializePermutationTable();
}

void NoiseGenerator::initializePermutationTable() {
    // Initialize with values 0-255
    std::vector<int> basePermutation(256);
    std::iota(basePermutation.begin(), basePermutation.end(), 0);
    
    // Shuffle the permutation table using our seeded RNG
    std::shuffle(basePermutation.begin(), basePermutation.end(), m_rng);
    
    // Duplicate the permutation table to avoid buffer overflow
    for (int i = 0; i < 256; ++i) {
        m_permutation[i] = m_permutation[i + 256] = basePermutation[i];
    }
}

float NoiseGenerator::fade(float t) {
    // Smoothstep function: 6t^5 - 15t^4 + 10t^3
    // This provides smooth interpolation with zero first and second derivatives at t=0 and t=1
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float NoiseGenerator::lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float NoiseGenerator::grad(int hash, float x, float y) const {
    // Use the last 2 bits to pick a gradient vector
    int h = hash & 3;
    float u = h < 2 ? x : -x;
    float v = h & 1 ? y : -y;
    return u + v;
}

float NoiseGenerator::grad(int hash, float x, float y, float z) const {
    // Use the last 4 bits to pick one of 12 gradient directions
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float NoiseGenerator::perlin2D(float x, float y) const {
    // Find unit square that contains point
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    
    // Find relative x, y of point in square
    x -= std::floor(x);
    y -= std::floor(y);
    
    // Compute fade curves for x and y
    float u = fade(x);
    float v = fade(y);
    
    // Hash coordinates of 4 square corners
    int A = m_permutation[X] + Y;
    int AA = m_permutation[A];
    int AB = m_permutation[A + 1];
    int B = m_permutation[X + 1] + Y;
    int BA = m_permutation[B];
    int BB = m_permutation[B + 1];
    
    // Blend results from 4 corners
    float res = lerp(
        lerp(grad(m_permutation[AA], x, y),
             grad(m_permutation[BA], x - 1, y), u),
        lerp(grad(m_permutation[AB], x, y - 1),
             grad(m_permutation[BB], x - 1, y - 1), u),
        v);
    
    return res;
}

float NoiseGenerator::perlin3D(float x, float y, float z) const {
    // Find unit cube that contains point
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    // Find relative x, y, z of point in cube
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    
    // Compute fade curves for x, y, z
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);
    
    // Hash coordinates of 8 cube corners
    int A = m_permutation[X] + Y;
    int AA = m_permutation[A] + Z;
    int AB = m_permutation[A + 1] + Z;
    int B = m_permutation[X + 1] + Y;
    int BA = m_permutation[B] + Z;
    int BB = m_permutation[B + 1] + Z;
    
    // Blend results from 8 corners of cube
    float res = lerp(
        lerp(
            lerp(grad(m_permutation[AA], x, y, z),
                 grad(m_permutation[BA], x - 1, y, z), u),
            lerp(grad(m_permutation[AB], x, y - 1, z),
                 grad(m_permutation[BB], x - 1, y - 1, z), u), v),
        lerp(
            lerp(grad(m_permutation[AA + 1], x, y, z - 1),
                 grad(m_permutation[BA + 1], x - 1, y, z - 1), u),
            lerp(grad(m_permutation[AB + 1], x, y - 1, z - 1),
                 grad(m_permutation[BB + 1], x - 1, y - 1, z - 1), u), v), w);
    
    return res;
}

float NoiseGenerator::fbm2D(float x, float y, int octaves, float persistence, float lacunarity) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;  // Used for normalizing result to [-1, 1]
    
    for (int i = 0; i < octaves; i++) {
        value += perlin2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

float NoiseGenerator::generateTerrainHeight(float x, float z, float scale, float heightScale) const {
    // Scale the coordinates for terrain generation
    float scaledX = x * scale;
    float scaledZ = z * scale;
    
    // Generate base terrain with multiple octaves
    float height = fbm2D(scaledX, scaledZ, 6, 0.5f, 2.0f);
    
    // Add additional detail layers
    // Ridged noise for mountain ridges
    float ridges = std::abs(fbm2D(scaledX * 0.5f, scaledZ * 0.5f, 4, 0.6f, 2.1f));
    ridges = 1.0f - ridges;  // Invert for ridge effect
    ridges = ridges * ridges;  // Square for sharper ridges
    
    // Fine detail noise
    float detail = fbm2D(scaledX * 4.0f, scaledZ * 4.0f, 3, 0.3f, 2.0f) * 0.1f;
    
    // Combine layers
    height = height * 0.7f + ridges * 0.2f + detail;
    
    // Apply height scaling and ensure positive heights
    height = (height * 0.5f + 0.5f) * heightScale;  // Convert from [-1,1] to [0, heightScale]
    
    return height;
}
