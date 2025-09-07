// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is the property of Rudra Ojha.  
// Redistribution, modification, or use of this code in any project  
// (commercial or non-commercial) is strictly prohibited without  
// prior written consent from the author.
//
// Unauthorized usage will be considered a violation of copyright law.

#include "TerrainGenerator.h"
#include "Mesh.h"
#include "NoiseGenerator.h"
#include <iostream>
#include <cmath>

std::shared_ptr<Mesh> TerrainGenerator::generateTerrainMesh(
    float width,
    float depth,
    unsigned int widthSegments,
    unsigned int depthSegments,
    const glm::vec3& center,
    float heightScale,
    float noiseScale,
    unsigned int seed) {
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    // Generate the displaced plane mesh with height variation
    if (!generateDisplacedPlaneMesh(width, depth, widthSegments, depthSegments, 
                                    center, heightScale, noiseScale, seed, vertices, indices)) {
        std::cerr << "Failed to generate displaced terrain mesh" << std::endl;
        return nullptr;
    }
    
    // Calculate smooth normals for proper lighting
    calculateSmoothNormals(vertices, indices);
    
    // Create and return the mesh
    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> TerrainGenerator::generateLowPolyTerrain(
    float size, const glm::vec3& center, float heightScale, unsigned int seed) {
    // Low-poly terrain with 50x50 subdivisions for performance
    return generateTerrainMesh(size, size, 50, 50, center, heightScale, 0.03f, seed);
}

std::shared_ptr<Mesh> TerrainGenerator::generateHighPolyTerrain(
    float size, const glm::vec3& center, float heightScale, unsigned int seed) {
    // High-poly terrain with 200x200 subdivisions for quality
    return generateTerrainMesh(size, size, 200, 200, center, heightScale, 0.02f, seed);
}

bool TerrainGenerator::generateDisplacedPlaneMesh(
    float width,
    float depth,
    unsigned int widthSegments,
    unsigned int depthSegments,
    const glm::vec3& center,
    float heightScale,
    float noiseScale,
    unsigned int seed,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices) {
    
    // Validate input parameters
    if (widthSegments < 1 || depthSegments < 1) {
        std::cerr << "TerrainGenerator: Invalid segment count. Must be at least 1." << std::endl;
        return false;
    }
    
    if (width <= 0.0f || depth <= 0.0f) {
        std::cerr << "TerrainGenerator: Invalid dimensions. Width and depth must be positive." << std::endl;
        return false;
    }
    
    // Calculate grid dimensions
    const unsigned int vertexCountX = widthSegments + 1;
    const unsigned int vertexCountZ = depthSegments + 1;
    const unsigned int totalVertices = vertexCountX * vertexCountZ;
    const unsigned int totalTriangles = widthSegments * depthSegments * 2;
    const unsigned int totalIndices = totalTriangles * 3;
    
    // Reserve memory for efficiency
    vertices.reserve(totalVertices);
    indices.reserve(totalIndices);
    
    // Create noise generator for height displacement
    NoiseGenerator noiseGen(seed);
    
    // Calculate step sizes
    const float stepX = width / static_cast<float>(widthSegments);
    const float stepZ = depth / static_cast<float>(depthSegments);
    
    // Calculate starting positions (centered around the center point)
    const float startX = center.x - width * 0.5f;
    const float startZ = center.z - depth * 0.5f;
    
    std::cout << "Generating terrain with " << totalVertices << " vertices..." << std::endl;
    
    // Generate vertices with height displacement
    for (unsigned int z = 0; z <= depthSegments; ++z) {
        for (unsigned int x = 0; x <= widthSegments; ++x) {
            Vertex vertex;
            
            // Calculate world position
            vertex.position.x = startX + static_cast<float>(x) * stepX;
            vertex.position.z = startZ + static_cast<float>(z) * stepZ;
            
            // Generate height using noise - this is the key difference!
            float terrainHeight = noiseGen.generateTerrainHeight(
                vertex.position.x, vertex.position.z, noiseScale, heightScale
            );
            vertex.position.y = center.y + terrainHeight;
            
            // Generate texture coordinates (0 to 1 range)
            vertex.texCoords = generateTexCoords(vertex.position, std::max(width, depth));
            
            // Initialize normal to point up (will be recalculated after mesh generation)
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            
            vertices.push_back(vertex);
        }
    }
    
    // Generate indices for triangle mesh
    // Each quad is made of two triangles
    for (unsigned int z = 0; z < depthSegments; ++z) {
        for (unsigned int x = 0; x < widthSegments; ++x) {
            // Calculate vertex indices for current quad
            const unsigned int topLeft = z * vertexCountX + x;
            const unsigned int topRight = topLeft + 1;
            const unsigned int bottomLeft = (z + 1) * vertexCountX + x;
            const unsigned int bottomRight = bottomLeft + 1;
            
            // First triangle (top-left, bottom-left, top-right)
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // Second triangle (top-right, bottom-left, bottom-right)
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    // Verify we generated the expected number of vertices and indices
    if (vertices.size() != totalVertices) {
        std::cerr << "TerrainGenerator: Vertex count mismatch. Expected: " 
                  << totalVertices << ", Got: " << vertices.size() << std::endl;
        return false;
    }
    
    if (indices.size() != totalIndices) {
        std::cerr << "TerrainGenerator: Index count mismatch. Expected: " 
                  << totalIndices << ", Got: " << indices.size() << std::endl;
        return false;
    }
    
    return true;
}

void TerrainGenerator::calculateSmoothNormals(
    std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices) {
    
    // Initialize all normals to zero
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }
    
    // Calculate face normals and accumulate to vertex normals
    for (size_t i = 0; i < indices.size(); i += 3) {
        const unsigned int i0 = indices[i];
        const unsigned int i1 = indices[i + 1];
        const unsigned int i2 = indices[i + 2];
        
        // Validate indices
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            std::cerr << "TerrainGenerator: Invalid index detected during normal calculation" << std::endl;
            continue;
        }
        
        // Get vertex positions
        const glm::vec3& v0 = vertices[i0].position;
        const glm::vec3& v1 = vertices[i1].position;
        const glm::vec3& v2 = vertices[i2].position;
        
        // Calculate face normal using cross product
        const glm::vec3 edge1 = v1 - v0;
        const glm::vec3 edge2 = v2 - v0;
        const glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
        
        // Accumulate face normal to all three vertices
        // This creates smooth normals by averaging adjacent face normals
        vertices[i0].normal += faceNormal;
        vertices[i1].normal += faceNormal;
        vertices[i2].normal += faceNormal;
    }
    
    // Normalize all vertex normals
    for (auto& vertex : vertices) {
        // Check for zero-length normals (shouldn't happen with proper mesh)
        const float length = glm::length(vertex.normal);
        if (length > 0.0f) {
            vertex.normal = vertex.normal / length; // Explicit normalization
        } else {
            // Fallback to upward-pointing normal
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            std::cerr << "TerrainGenerator: Warning - zero-length normal detected, using fallback" << std::endl;
        }
    }
}

glm::vec2 TerrainGenerator::generateTexCoords(const glm::vec3& worldPos, float terrainSize) {
    // Map world position to UV coordinates (0 to 1 range)
    // This allows for proper texture coordinate calculation in the shader
    const float u = (worldPos.x + terrainSize * 0.5f) / terrainSize;
    const float v = (worldPos.z + terrainSize * 0.5f) / terrainSize;
    
    // Clamp to ensure valid texture coordinate range
    return glm::vec2(
        glm::clamp(u, 0.0f, 1.0f),
        glm::clamp(v, 0.0f, 1.0f)
    );
}
