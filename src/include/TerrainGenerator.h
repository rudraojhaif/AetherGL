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

#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
class Mesh;
struct Vertex;
class NoiseGenerator;

/**
 * TerrainGenerator - Creates procedural terrain meshes for rendering
 * 
 * This class generates high-resolution plane meshes that serve as the base geometry
 * for procedural terrain generation in the fragment shader. The terrain height and
 * materials are calculated entirely in the GPU using fractal Brownian motion noise.
 * 
 * Industry standard practices implemented:
 * - Efficient vertex buffer layout
 * - Proper normal vector calculation for lighting
 * - Configurable mesh resolution for performance scaling
 * - Memory-efficient index buffer usage
 * - Clean RAII resource management
 */
class TerrainGenerator {
public:
    /**
     * Generate a terrain mesh with the specified parameters
     * 
     * @param width World-space width of the terrain
     * @param depth World-space depth of the terrain
     * @param widthSegments Number of subdivisions along the width (higher = more detail)
     * @param depthSegments Number of subdivisions along the depth (higher = more detail)
     * @param center Center position of the terrain in world space
     * @param heightScale Maximum height variation of the terrain
     * @param noiseScale Scale factor for noise sampling (smaller = more zoomed out)
     * @param seed Random seed for reproducible terrain (0 = random)
     * @return Shared pointer to the generated mesh, or nullptr on failure
     */
    static std::shared_ptr<Mesh> generateTerrainMesh(
        float width = 20.0f,
        float depth = 20.0f,
        unsigned int widthSegments = 100,
        unsigned int depthSegments = 100,
        const glm::vec3& center = glm::vec3(0.0f, 0.0f, 0.0f),
        float heightScale = 10.0f,
        float noiseScale = 0.05f,
        unsigned int seed = 0
    );

    /**
     * Generate a terrain mesh optimized for performance
     * Uses lower subdivision count suitable for real-time rendering
     */
    static std::shared_ptr<Mesh> generateLowPolyTerrain(
        float size = 20.0f,
        const glm::vec3& center = glm::vec3(0.0f, 0.0f, 0.0f),
        float heightScale = 8.0f,
        unsigned int seed = 0
    );

    /**
     * Generate a terrain mesh optimized for quality
     * Uses higher subdivision count for detailed terrain rendering
     */
    static std::shared_ptr<Mesh> generateHighPolyTerrain(
        float size = 20.0f,
        const glm::vec3& center = glm::vec3(0.0f, 0.0f, 0.0f),
        float heightScale = 12.0f,
        unsigned int seed = 0
    );

private:
    /**
     * Internal mesh generation implementation
     * Creates vertices and indices for a subdivided plane mesh with height displacement
     */
    static bool generateDisplacedPlaneMesh(
        float width,
        float depth,
        unsigned int widthSegments,
        unsigned int depthSegments,
        const glm::vec3& center,
        float heightScale,
        float noiseScale,
        unsigned int seed,
        std::vector<Vertex>& vertices,
        std::vector<unsigned int>& indices
    );

    /**
     * Calculate smooth normals for terrain vertices
     * Uses averaged face normals for smooth lighting across the mesh
     */
    static void calculateSmoothNormals(
        std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices
    );

    /**
     * Generate texture coordinates based on world position
     * Maps world space coordinates to UV space for texture sampling
     */
    static glm::vec2 generateTexCoords(const glm::vec3& worldPos, float terrainSize);
};
