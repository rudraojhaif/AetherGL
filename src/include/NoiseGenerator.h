// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is the property of Rudra Ojha.  
// Redistribution, modification, or use of this code in any project  
// (commercial or non-commercial) is strictly prohibited without  
// prior written consent from the author.
//
// Unauthorized usage will be considered a violation of copyright law.

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>

/**
 * NoiseGenerator - CPU-based procedural noise generation
 * 
 * This class provides industry-standard noise generation algorithms
 * optimized for terrain generation. It implements Perlin noise and
 * fractal Brownian motion (fBm) for creating realistic terrain heightmaps.
 * 
 * Features:
 * - High-quality Perlin noise implementation
 * - Fractal Brownian Motion (fBm) for natural terrain patterns
 * - Configurable octaves, persistence, and lacunarity
 * - Optimized for real-time terrain generation
 * - Seed-based generation for reproducible results
 */
class NoiseGenerator {
public:
    /**
     * Constructor with optional seed for reproducible noise
     * @param seed Random seed for noise generation (0 = random seed)
     */
    explicit NoiseGenerator(unsigned int seed = 0);

    /**
     * Generate 2D Perlin noise at the given coordinates
     * @param x X coordinate
     * @param y Y coordinate (often used as Z in world space)
     * @return Noise value in range [-1, 1]
     */
    float perlin2D(float x, float y) const;

    /**
     * Generate 3D Perlin noise at the given coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return Noise value in range [-1, 1]
     */
    float perlin3D(float x, float y, float z) const;

    /**
     * Generate fractal Brownian motion (fBm) noise
     * Combines multiple octaves of Perlin noise for natural-looking terrain
     * 
     * @param x X coordinate
     * @param y Y coordinate (often used as Z in world space)
     * @param octaves Number of noise octaves (higher = more detail)
     * @param persistence Amplitude multiplier for each octave (typically 0.5)
     * @param lacunarity Frequency multiplier for each octave (typically 2.0)
     * @return fBm noise value in range [-1, 1]
     */
    float fbm2D(float x, float y, int octaves = 6, float persistence = 0.5f, float lacunarity = 2.0f) const;

    /**
     * Generate terrain height using optimized fBm parameters
     * This is specifically tuned for realistic terrain generation
     * 
     * @param x World X coordinate
     * @param z World Z coordinate
     * @param scale Overall terrain scale (smaller = more zoomed out)
     * @param heightScale Height multiplier for final result
     * @return Terrain height value
     */
    float generateTerrainHeight(float x, float z, float scale = 0.01f, float heightScale = 10.0f) const;

    /**
     * Set the seed for noise generation
     * @param seed New seed value (0 = random seed)
     */
    void setSeed(unsigned int seed);

private:
    // Permutation table for Perlin noise
    std::vector<int> m_permutation;
    
    // Random number generator for seed initialization
    mutable std::mt19937 m_rng;

    /**
     * Initialize the permutation table with the current seed
     */
    void initializePermutationTable();

    /**
     * Fade function for smooth interpolation (6t^5 - 15t^4 + 10t^3)
     */
    static float fade(float t);

    /**
     * Linear interpolation
     */
    static float lerp(float a, float b, float t);

    /**
     * Gradient function for Perlin noise
     */
    float grad(int hash, float x, float y) const;
    float grad(int hash, float x, float y, float z) const;
};
