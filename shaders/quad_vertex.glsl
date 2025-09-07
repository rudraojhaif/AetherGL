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

// Simple vertex shader for full-screen quad rendering
// Used by all post-processing fragment shaders

layout (location = 0) in vec2 aPos;        // Quad vertex positions
layout (location = 1) in vec2 aTexCoord;   // Texture coordinates

out vec2 TexCoord; // Pass texture coordinates to fragment shader

void main() {
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0); // Simple pass-through for full-screen quad
}
