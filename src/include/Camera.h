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
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
private:
    // Camera attributes
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    // Euler angles
    float m_yaw;
    float m_pitch;

    // Camera options
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;

    void updateCameraVectors();

public:
    // Constructor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);

    // Returns view matrix
    glm::mat4 getViewMatrix() const;

    // Process input
    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void processMouseScroll(float yOffset);

    // Getters
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getFront() const { return m_front; }
    float getZoom() const { return m_zoom; }
    float getPitch() const { return m_pitch; }
    float getYaw() const { return m_yaw; }
    
    // Setters
    void setPitch(float pitch) { 
        m_pitch = pitch; 
        updateCameraVectors(); 
    }
    void setYaw(float yaw) { 
        m_yaw = yaw; 
        updateCameraVectors(); 
    }
    void setPosition(const glm::vec3& position) {
        m_position = position;
    }
};