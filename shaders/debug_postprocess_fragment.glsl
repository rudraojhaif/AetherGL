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

// Debug version of final post-processing shader
// Just passes through the scene texture to isolate rendering issues

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_sceneTexture;    // Main scene color

void main() {
    // Simple pass-through - just sample the scene texture
    vec3 color = texture(u_sceneTexture, TexCoord).rgb;
    
    // Output the color directly without any effects
    FragColor = vec4(color, 1.0);
}
