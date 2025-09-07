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
#include <memory>

class Shader;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class Mesh {
private:
    unsigned int m_VAO, m_VBO, m_EBO;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    
    void setupMesh();

public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();
    
    void draw(std::shared_ptr<Shader> shader);
    
    // Getters
    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    const std::vector<unsigned int>& getIndices() const { return m_indices; }
};