#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
class Mesh;
struct Vertex;

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
     * @return Shared pointer to the generated mesh, or nullptr on failure
     */
    static std::shared_ptr<Mesh> generateTerrainMesh(
        float width = 20.0f,
        float depth = 20.0f,
        unsigned int widthSegments = 100,
        unsigned int depthSegments = 100,
        const glm::vec3& center = glm::vec3(0.0f, 0.0f, 0.0f)
    );

    /**
     * Generate a terrain mesh optimized for performance
     * Uses lower subdivision count suitable for real-time rendering
     */
    static std::shared_ptr<Mesh> generateLowPolyTerrain(
        float size = 20.0f,
        const glm::vec3& center = glm::vec3(0.0f, 0.0f, 0.0f)
    );

    /**
     * Generate a terrain mesh optimized for quality
     * Uses higher subdivision count for detailed terrain rendering
     */
    static std::shared_ptr<Mesh> generateHighPolyTerrain(
        float size = 20.0f,
        const glm::vec3& center = glm::vec3(0.0f, 0.0f, 0.0f)
    );

private:
    /**
     * Internal mesh generation implementation
     * Creates vertices and indices for a subdivided plane mesh
     */
    static bool generatePlaneMesh(
        float width,
        float depth,
        unsigned int widthSegments,
        unsigned int depthSegments,
        const glm::vec3& center,
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
