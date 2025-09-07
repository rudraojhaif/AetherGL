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
 * ObjWriter.h - Wavefront OBJ file writer for terrain mesh export
 * 
 * This class provides functionality to export generated terrain meshes to the
 * industry-standard Wavefront OBJ file format. The OBJ format is widely supported
 * by 3D modeling software and game engines for mesh interchange.
 * 
 * Features:
 * - Export vertex positions, normals, and texture coordinates
 * - Support for indexed triangle meshes
 * - Clean, standards-compliant OBJ format output
 * - Automatic file path sanitization and error handling
 * - Memory-efficient streaming write operations
 * 
 * OBJ Format Structure:
 * - v x y z          (vertex positions)
 * - vt u v           (texture coordinates) 
 * - vn x y z         (vertex normals)
 * - f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3  (triangular faces)
 */

#pragma once

#include <string>
#include <memory>
#include <vector>

// Forward declarations
class Mesh;
struct Vertex;

/**
 * ObjWriter - Exports terrain meshes to Wavefront OBJ format
 * 
 * This utility class handles the conversion of internal mesh data structures
 * to the standardized OBJ file format, enabling terrain export for external
 * use in modeling software, game engines, or other applications.
 * 
 * Performance considerations:
 * - Uses buffered file I/O for efficient large mesh export
 * - Minimizes memory allocations during write operations
 * - Provides progress feedback for large terrain exports
 */
class ObjWriter {
public:
    /**
     * Export terrain mesh to OBJ file
     * 
     * Writes the complete mesh data (vertices, normals, texture coordinates,
     * and triangle indices) to a Wavefront OBJ file. The file will be created
     * or overwritten at the specified path.
     * 
     * OBJ file structure generated:
     * 1. Header comment with generation timestamp
     * 2. All vertex positions (v x y z)
     * 3. All texture coordinates (vt u v)  
     * 4. All vertex normals (vn x y z)
     * 5. All triangular faces with indices (f v1/vt1/vn1 ...)
     * 
     * @param mesh Shared pointer to the mesh to export
     * @param filename Output filename (should end with .obj)
     * @param meshName Optional name for the object in the OBJ file
     * @return true if export successful, false on error
     * 
     * @note The function will create directories in the path if they don't exist
     * @note Existing files will be overwritten without warning
     */
    static bool exportMesh(
        std::shared_ptr<Mesh> mesh,
        const std::string& filename,
        const std::string& meshName = "TerrainMesh"
    );

    /**
     * Export terrain mesh with additional metadata
     * 
     * Extended export function that includes additional metadata in the OBJ file
     * such as generation parameters, vertex count statistics, and terrain bounds.
     * 
     * @param mesh Shared pointer to the mesh to export
     * @param filename Output filename (should end with .obj)
     * @param meshName Name for the object in the OBJ file
     * @param terrainWidth World-space width of the terrain
     * @param terrainDepth World-space depth of the terrain  
     * @param heightScale Maximum height variation used in generation
     * @param seed Random seed used for terrain generation (0 if random)
     * @return true if export successful, false on error
     */
    static bool exportTerrainMesh(
        std::shared_ptr<Mesh> mesh,
        const std::string& filename,
        const std::string& meshName,
        float terrainWidth,
        float terrainDepth,
        float heightScale,
        unsigned int seed = 0
    );

    /**
     * Validate OBJ filename and path
     * 
     * Checks if the provided filename is valid for OBJ export and creates
     * necessary directories if they don't exist.
     * 
     * @param filename Filename to validate
     * @return true if filename is valid and path is accessible
     */
    static bool validateFilename(const std::string& filename);

    /**
     * Get mesh statistics for export reporting
     * 
     * Analyzes the mesh to provide statistics useful for export reporting
     * and debugging. Information includes vertex count, triangle count,
     * bounding box dimensions, and estimated file size.
     * 
     * @param mesh Mesh to analyze
     * @return Statistics string suitable for console output or logging
     */
    static std::string getMeshStatistics(std::shared_ptr<Mesh> mesh);

private:
    /**
     * Write OBJ file header with metadata
     * 
     * Writes standardized header comment block including:
     * - Generator application name and version
     * - Export timestamp
     * - Mesh statistics (vertex/face counts)
     * - Optional terrain generation parameters
     * 
     * @param file Output file stream
     * @param meshName Name of the exported mesh
     * @param vertexCount Number of vertices in the mesh
     * @param faceCount Number of triangular faces in the mesh
     */
    static void writeHeader(
        std::ofstream& file,
        const std::string& meshName,
        size_t vertexCount,
        size_t faceCount
    );

    /**
     * Write vertex data to OBJ file
     * 
     * Outputs all vertex positions, texture coordinates, and normals
     * in the standard OBJ format. Uses efficient streaming I/O with
     * appropriate precision for coordinate values.
     * 
     * @param file Output file stream
     * @param vertices Vector of vertex data to write
     */
    static void writeVertexData(
        std::ofstream& file,
        const std::vector<Vertex>& vertices
    );

    /**
     * Write face indices to OBJ file
     * 
     * Converts triangle index data to OBJ face format. Each triangle
     * is written as "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3" where indices
     * are 1-based as per OBJ specification.
     * 
     * @param file Output file stream  
     * @param indices Vector of triangle indices (3 indices per triangle)
     */
    static void writeFaceData(
        std::ofstream& file,
        const std::vector<unsigned int>& indices
    );

    /**
     * Create directory path for file
     * 
     * Creates all necessary directories in the file path if they don't exist.
     * Handles both relative and absolute paths correctly.
     * 
     * @param filepath Full file path including filename
     * @return true if directories exist or were created successfully
     */
    static bool createDirectoryPath(const std::string& filepath);

    /**
     * Get current timestamp string
     * 
     * @return Formatted timestamp string for file headers
     */
    static std::string getCurrentTimestamp();
};