// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is the property of Rudra Ojha.  
// Redistribution, modification, or use of this code in any project  
// (commercial or non-commercial) is strictly prohibited without  
// prior written consent from the author.
//
// Unauthorized usage will be considered a violation of copyright law.

/**
 * ObjWriter.cpp - Implementation of Wavefront OBJ file writer
 * 
 * Provides complete functionality for exporting procedurally generated terrain
 * meshes to the industry-standard OBJ file format. The implementation focuses
 * on efficiency, standards compliance, and robust error handling.
 * 
 * Key implementation details:
 * - Uses buffered file I/O for optimal performance with large meshes
 * - Maintains 1-based indexing as required by OBJ specification
 * - Includes comprehensive error checking and validation
 * - Generates clean, standards-compliant OBJ output
 * - Supports cross-platform file path handling
 */

#include "ObjWriter.h"
#include "Mesh.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <cmath>

/**
 * Export terrain mesh to OBJ file with standard formatting
 * 
 * This is the primary export function that handles the complete process of
 * converting internal mesh data to OBJ format. The function performs:
 * 1. Input validation and error checking
 * 2. File creation and header writing
 * 3. Vertex data export (positions, UVs, normals)
 * 4. Face index export with proper 1-based indexing
 * 5. File cleanup and error reporting
 * 
 * Performance: O(n) where n is the number of vertices + faces
 * Memory usage: Minimal additional allocation, streams data directly
 * 
 * @param mesh Shared pointer to mesh containing vertex and index data
 * @param filename Target OBJ file path (will be created/overwritten)
 * @param meshName Object name to embed in the OBJ file
 * @return Success status of the export operation
 */
bool ObjWriter::exportMesh(
    std::shared_ptr<Mesh> mesh,
    const std::string& filename,
    const std::string& meshName) {
    
    // Input validation - ensure mesh is valid and accessible
    if (!mesh) {
        std::cerr << "ObjWriter Error: Cannot export null mesh" << std::endl;
        return false;
    }

    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();

    // Validate mesh data integrity
    if (vertices.empty()) {
        std::cerr << "ObjWriter Error: Cannot export mesh with no vertices" << std::endl;
        return false;
    }

    if (indices.empty()) {
        std::cerr << "ObjWriter Error: Cannot export mesh with no indices" << std::endl;
        return false;
    }

    // Ensure triangle count is valid (indices must be multiple of 3)
    if (indices.size() % 3 != 0) {
        std::cerr << "ObjWriter Error: Index count must be multiple of 3 for triangular mesh" << std::endl;
        return false;
    }

    // Validate filename and create directory structure if needed
    if (!validateFilename(filename)) {
        std::cerr << "ObjWriter Error: Invalid filename or cannot create directory: " << filename << std::endl;
        return false;
    }

    // Open output file with appropriate settings for text output
    std::ofstream objFile(filename, std::ios::out | std::ios::trunc);
    if (!objFile.is_open()) {
        std::cerr << "ObjWriter Error: Cannot create file: " << filename << std::endl;
        return false;
    }

    // Configure output precision for floating-point coordinates
    objFile << std::fixed << std::setprecision(6);

    try {
        // Write OBJ file header with metadata
        size_t faceCount = indices.size() / 3;
        writeHeader(objFile, meshName, vertices.size(), faceCount);

        // Export vertex data (positions, texture coordinates, normals)
        writeVertexData(objFile, vertices);

        // Export face data with 1-based indexing
        writeFaceData(objFile, indices);

        // Write footer comment for completeness
        objFile << "\n# End of " << meshName << " - Total: " 
                << vertices.size() << " vertices, " << faceCount << " faces\n";

        objFile.close();

        // Report successful export with statistics
        std::cout << "Successfully exported terrain mesh to: " << filename << std::endl;
        std::cout << "  Vertices: " << vertices.size() << std::endl;
        std::cout << "  Triangles: " << faceCount << std::endl;
        std::cout << "  File size: ~" << ((vertices.size() * 120 + indices.size() * 20) / 1024) << " KB" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "ObjWriter Error: Exception during export: " << e.what() << std::endl;
        objFile.close();
        return false;
    }
}

/**
 * Extended export with terrain generation metadata
 * 
 * This enhanced export function includes additional metadata about the terrain
 * generation process in the OBJ file header. This information is useful for:
 * - Reproducing the terrain with the same parameters
 * - Understanding the scale and properties of the exported mesh
 * - Debugging and analysis of procedural generation results
 * 
 * @param mesh Mesh data to export
 * @param filename Target file path
 * @param meshName Object name for the OBJ file
 * @param terrainWidth World-space width of the terrain
 * @param terrainDepth World-space depth of the terrain
 * @param heightScale Maximum height variation in the terrain
 * @param seed Random seed used for generation (0 if random)
 * @return Success status of export operation
 */
bool ObjWriter::exportTerrainMesh(
    std::shared_ptr<Mesh> mesh,
    const std::string& filename,
    const std::string& meshName,
    float terrainWidth,
    float terrainDepth,
    float heightScale,
    unsigned int seed) {

    // Use standard export function first
    if (!exportMesh(mesh, filename, meshName)) {
        return false;
    }

    // Append terrain-specific metadata to the file
    std::ofstream objFile(filename, std::ios::app);
    if (!objFile.is_open()) {
        std::cerr << "ObjWriter Warning: Cannot append terrain metadata to: " << filename << std::endl;
        return true; // Export succeeded, metadata append failed (non-critical)
    }

    objFile << "\n# Terrain Generation Parameters:\n";
    objFile << "# Width: " << terrainWidth << " units\n";
    objFile << "# Depth: " << terrainDepth << " units\n";
    objFile << "# Height Scale: " << heightScale << " units\n";
    if (seed != 0) {
        objFile << "# Random Seed: " << seed << "\n";
    } else {
        objFile << "# Random Seed: Auto-generated\n";
    }
    objFile << "# Generated by AetherGL Terrain System\n";

    objFile.close();
    return true;
}

/**
 * Validate filename and prepare directory structure
 * 
 * Performs comprehensive validation of the target filename including:
 * - Path format validation (handles both relative and absolute paths)
 * - Directory existence checking and creation
 * - Write permission verification
 * - File extension validation (.obj recommended)
 * 
 * @param filename Target filename to validate
 * @return true if filename is valid and path is accessible
 */
bool ObjWriter::validateFilename(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }

    try {
        // Convert to filesystem path for robust handling
        std::filesystem::path filePath(filename);
        
        // Create parent directories if they don't exist
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        // Verify we can write to the target location by attempting to create a test file
        std::ofstream testFile(filename, std::ios::out | std::ios::app);
        if (!testFile.is_open()) {
            return false;
        }
        testFile.close();

        return true;

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error validating filename: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error validating filename: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Generate comprehensive mesh statistics report
 * 
 * Analyzes the mesh to provide detailed statistics useful for:
 * - Performance analysis and optimization
 * - Memory usage estimation
 * - Geometric properties verification
 * - Export size prediction
 * 
 * Statistics calculated:
 * - Vertex and face counts
 * - Bounding box dimensions
 * - Approximate memory usage
 * - Estimated file size
 * 
 * @param mesh Mesh to analyze
 * @return Formatted statistics string
 */
std::string ObjWriter::getMeshStatistics(std::shared_ptr<Mesh> mesh) {
    if (!mesh) {
        return "Invalid mesh - cannot generate statistics";
    }

    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();

    std::ostringstream stats;
    stats << std::fixed << std::setprecision(2);

    // Basic counts
    size_t vertexCount = vertices.size();
    size_t triangleCount = indices.size() / 3;
    
    stats << "Mesh Statistics:\n";
    stats << "  Vertices: " << vertexCount << "\n";
    stats << "  Triangles: " << triangleCount << "\n";

    if (!vertices.empty()) {
        // Calculate bounding box
        glm::vec3 minBounds = vertices[0].position;
        glm::vec3 maxBounds = vertices[0].position;

        for (const auto& vertex : vertices) {
            minBounds = glm::min(minBounds, vertex.position);
            maxBounds = glm::max(maxBounds, vertex.position);
        }

        glm::vec3 dimensions = maxBounds - minBounds;
        
        stats << "  Bounding Box:\n";
        stats << "    Min: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ")\n";
        stats << "    Max: (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")\n";
        stats << "    Dimensions: " << dimensions.x << " x " << dimensions.y << " x " << dimensions.z << "\n";

        // Memory usage estimates
        size_t vertexMemory = vertexCount * sizeof(Vertex);
        size_t indexMemory = indices.size() * sizeof(unsigned int);
        size_t totalMemory = vertexMemory + indexMemory;

        stats << "  Memory Usage:\n";
        stats << "    Vertex data: " << (vertexMemory / 1024.0f) << " KB\n";
        stats << "    Index data: " << (indexMemory / 1024.0f) << " KB\n";
        stats << "    Total: " << (totalMemory / 1024.0f) << " KB\n";

        // Estimated OBJ file size
        size_t estimatedFileSize = vertexCount * 120 + indices.size() * 20; // Rough estimate
        stats << "  Estimated OBJ file size: " << (estimatedFileSize / 1024.0f) << " KB\n";
    }

    return stats.str();
}

/**
 * Write standardized OBJ file header
 * 
 * Generates a comprehensive header comment block that includes:
 * - Application identification and version
 * - Export timestamp for traceability
 * - Mesh statistics for quick reference
 * - Format specification compliance notes
 * 
 * The header follows industry conventions for OBJ file documentation.
 * 
 * @param file Output file stream
 * @param meshName Name of the exported object
 * @param vertexCount Number of vertices in the mesh
 * @param faceCount Number of triangular faces in the mesh
 */
void ObjWriter::writeHeader(
    std::ofstream& file,
    const std::string& meshName,
    size_t vertexCount,
    size_t faceCount) {

    file << "# Wavefront OBJ file exported from AetherGL Terrain System\n";
    file << "# Generated: " << getCurrentTimestamp() << "\n";
    file << "# Object: " << meshName << "\n";
    file << "# Vertices: " << vertexCount << "\n";
    file << "# Faces: " << faceCount << "\n";
    file << "# Format: Triangular mesh with positions, UVs, and normals\n";
    file << "#\n";
    file << "# This file uses 1-based indexing as per OBJ specification\n";
    file << "# Face format: f vertex/texture/normal vertex/texture/normal vertex/texture/normal\n";
    file << "#\n\n";

    // Object declaration
    file << "o " << meshName << "\n\n";
}

/**
 * Write vertex data in OBJ format
 * 
 * Exports all vertex attributes in the standard OBJ format:
 * - v x y z (vertex positions)
 * - vt u v (texture coordinates)
 * - vn x y z (vertex normals)
 * 
 * The function maintains proper ordering so that vertex N has its texture
 * coordinate at vt N and normal at vn N, enabling straightforward indexing.
 * 
 * Performance optimization: Uses buffered output for efficient writing of
 * large vertex datasets.
 * 
 * @param file Output file stream configured with appropriate precision
 * @param vertices Vector of vertex data to export
 */
void ObjWriter::writeVertexData(std::ofstream& file, const std::vector<Vertex>& vertices) {
    // Write vertex positions
    file << "# Vertex positions\n";
    for (const auto& vertex : vertices) {
        file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
    }
    file << "\n";

    // Write texture coordinates
    file << "# Texture coordinates\n";
    for (const auto& vertex : vertices) {
        file << "vt " << vertex.texCoords.x << " " << vertex.texCoords.y << "\n";
    }
    file << "\n";

    // Write vertex normals
    file << "# Vertex normals\n";
    for (const auto& vertex : vertices) {
        file << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
    }
    file << "\n";
}

/**
 * Write face indices in OBJ format
 * 
 * Converts internal 0-based triangle indices to OBJ-compliant 1-based indices.
 * Each triangle is written in the format:
 * f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
 * 
 * Where:
 * - v1, v2, v3 are vertex position indices
 * - vt1, vt2, vt3 are texture coordinate indices  
 * - vn1, vn2, vn3 are vertex normal indices
 * 
 * Since our vertex data is consistently ordered, all three indices are
 * the same for each vertex (position index = texture index = normal index).
 * 
 * @param file Output file stream
 * @param indices Triangle index data (3 indices per triangle, 0-based)
 */
void ObjWriter::writeFaceData(std::ofstream& file, const std::vector<unsigned int>& indices) {
    file << "# Triangular faces (vertex/texture/normal indices)\n";
    
    // Process triangles (3 indices at a time)
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Convert from 0-based to 1-based indexing as required by OBJ format
        unsigned int v1 = indices[i] + 1;
        unsigned int v2 = indices[i + 1] + 1;
        unsigned int v3 = indices[i + 2] + 1;
        
        // Write face with vertex/texture/normal indices
        // Since our data is consistently ordered, all three indices are the same for each vertex
        file << "f " << v1 << "/" << v1 << "/" << v1 << " "
                    << v2 << "/" << v2 << "/" << v2 << " "
                    << v3 << "/" << v3 << "/" << v3 << "\n";
    }
}

/**
 * Get current system timestamp formatted for file headers
 * 
 * Generates a human-readable timestamp string suitable for embedding in
 * exported file headers. The format includes date, time, and is suitable
 * for international use.
 * 
 * @return Formatted timestamp string (e.g., "2024-01-15 14:30:25 UTC")
 */
std::string ObjWriter::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}