// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is the property of Rudra Ojha.  
// Redistribution, modification, or use of this code in any project  
// (commercial or non-commercial) is strictly prohibited without  
// prior written consent from the author.
//
// Unauthorized usage will be considered a violation of copyright law.

#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;
    
    // Remove translation from view matrix for skybox
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    
    // Ensure skybox is always at far plane
    gl_Position = pos.xyww;
}
