#include "ObjLoader.h"
#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

std::shared_ptr<Mesh> ObjLoader::loadObjFile(const std::string& filepath, float scale) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    if (!parseObjFile(filepath, vertices, indices, scale)) {
        std::cerr << "Failed to load OBJ file: " << filepath << std::endl;
        return nullptr;
    }

    // Calculate normals if they weren't provided
    calculateNormals(vertices, indices);

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> ObjLoader::loadObjFile(const std::string& filepath, const glm::vec3& scale) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    if (!parseObjFile(filepath, vertices, indices, scale)) {
        std::cerr << "Failed to load OBJ file: " << filepath << std::endl;
        return nullptr;
    }

    // Calculate normals if they weren't provided
    calculateNormals(vertices, indices);

    return std::make_shared<Mesh>(vertices, indices);
}

// Original function for backward compatibility
std::shared_ptr<Mesh> ObjLoader::loadObjFile(const std::string& filepath) {
    return loadObjFile(filepath, 1.0f);
}

bool ObjLoader::parseObjFile(const std::string& filepath,
                            std::vector<Vertex>& vertices,
                            std::vector<unsigned int>& indices,
                            float scale) {
    return parseObjFile(filepath, vertices, indices, glm::vec3(scale));
}

bool ObjLoader::parseObjFile(const std::string& filepath,
                            std::vector<Vertex>& vertices,
                            std::vector<unsigned int>& indices,
                            const glm::vec3& scale) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::unordered_map<std::string, unsigned int> vertexMap;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            // Vertex position - apply scale here
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            pos.x *= scale.x;
            pos.y *= scale.y;
            pos.z *= scale.z;
            positions.push_back(pos);
        }
        else if (prefix == "vn") {
            // Vertex normal (don't scale normals, they should remain unit vectors)
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "vt") {
            // Texture coordinate
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            texCoords.push_back(tex);
        }
        else if (prefix == "f") {
            // Face
            std::string vertex1, vertex2, vertex3;
            iss >> vertex1 >> vertex2 >> vertex3;

            std::vector<std::string> faceVertices = {vertex1, vertex2, vertex3};

            for (const auto& vertexStr : faceVertices) {
                if (vertexMap.find(vertexStr) == vertexMap.end()) {
                    // Parse vertex string (format: pos/tex/normal or pos//normal or pos/tex or pos)
                    std::istringstream viss(vertexStr);
                    std::string posStr, texStr, normalStr;

                    std::getline(viss, posStr, '/');
                    std::getline(viss, texStr, '/');
                    std::getline(viss, normalStr);

                    Vertex vertex;

                    // Position (required) - already scaled when loaded
                    if (!posStr.empty()) {
                        int posIndex = std::stoi(posStr) - 1; // OBJ indices are 1-based
                        if (posIndex >= 0 && posIndex < positions.size()) {
                            vertex.position = positions[posIndex];
                        }
                    }

                    // Texture coordinates (optional)
                    if (!texStr.empty()) {
                        int texIndex = std::stoi(texStr) - 1;
                        if (texIndex >= 0 && texIndex < texCoords.size()) {
                            vertex.texCoords = texCoords[texIndex];
                        }
                    }

                    // Normal (optional)
                    if (!normalStr.empty()) {
                        int normalIndex = std::stoi(normalStr) - 1;
                        if (normalIndex >= 0 && normalIndex < normals.size()) {
                            vertex.normal = normals[normalIndex];
                        }
                    }

                    vertexMap[vertexStr] = vertices.size();
                    vertices.push_back(vertex);
                }

                indices.push_back(vertexMap[vertexStr]);
            }
        }
    }

    file.close();
    return true;
}

// Original function for backward compatibility
bool ObjLoader::parseObjFile(const std::string& filepath,
                            std::vector<Vertex>& vertices,
                            std::vector<unsigned int>& indices) {
    return parseObjFile(filepath, vertices, indices, 1.0f);
}

void ObjLoader::calculateNormals(std::vector<Vertex>& vertices,
                                const std::vector<unsigned int>& indices) {
    // Initialize all normals to zero
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }
    
    // Calculate face normals and accumulate
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
        
        vertices[i0].normal += faceNormal;
        vertices[i1].normal += faceNormal;
        vertices[i2].normal += faceNormal;
    }
    
    // Normalize all vertex normals
    for (auto& vertex : vertices) {
        if (glm::length(vertex.normal) > 0.0f) {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }
}