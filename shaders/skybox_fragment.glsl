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

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float exposure;

void main() {    
    vec3 envColor = texture(skybox, TexCoords).rgb;
    
    // HDR tone mapping
    envColor *= exposure;
    envColor = envColor / (envColor + vec3(1.0));
    
    // Gamma correction
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    FragColor = vec4(envColor, 1.0);
}
