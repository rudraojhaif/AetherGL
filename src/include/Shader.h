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

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
private:
    unsigned int m_program;
    
    std::string readFile(const std::string& filepath);
    void checkCompileErrors(unsigned int shader, const std::string& type);

public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    
    void use();
    
    // Utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    
    unsigned int getProgram() const { return m_program; }
};