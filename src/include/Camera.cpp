/**
 * Camera.cpp - First-person camera implementation for 3D navigation
 * 
 * Implements a free-flying first-person camera with:
 * - Euler angle rotation system (yaw, pitch)
 * - WASD movement controls
 * - Mouse look functionality
 * - Smooth zoom controls via mouse wheel
 * - Optimized view matrix computation using GLM's lookAt
 */

#include "Camera.h"
#include <algorithm>    // For std::clamp function

/**
 * Default camera constants
 * These values provide reasonable defaults for typical 3D scenes
 */
const float YAW = -90.0f;       // Initial yaw angle (facing negative Z-axis)
const float PITCH = 0.0f;       // Initial pitch angle (looking straight ahead)  
const float SPEED = 2.5f;       // Movement speed in units per second
const float SENSITIVITY = 0.1f; // Mouse sensitivity multiplier
const float ZOOM = 45.0f;       // Field of view in degrees

/**
 * Camera constructor
 * Initializes camera with specified position, up vector, and rotation angles
 * 
 * Mathematical basis:
 * - Uses right-handed coordinate system (OpenGL standard)
 * - Y-axis points up, Z-axis points toward viewer, X-axis points right
 * - Yaw rotation around Y-axis, pitch rotation around X-axis
 * 
 * @param position Initial world position of camera
 * @param up World up vector (typically 0, 1, 0)
 * @param yaw Initial horizontal rotation in degrees
 * @param pitch Initial vertical rotation in degrees  
 */
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_position(position)
    , m_worldUp(up)
    , m_yaw(yaw)
    , m_pitch(pitch)
    , m_movementSpeed(SPEED)
    , m_mouseSensitivity(SENSITIVITY)
    , m_zoom(ZOOM)
{
    updateCameraVectors(); // Calculate initial camera orientation vectors
}

/**
 * Generate view matrix for rendering
 * 
 * The view matrix transforms world coordinates to camera space
 * Mathematical formula: V = lookAt(eye, center, up)
 * Where:
 * - eye: camera position in world space
 * - center: point the camera is looking at (position + front vector)
 * - up: camera's up vector (perpendicular to front and right)
 * 
 * This creates a right-handed coordinate system where:
 * - Positive X points to the camera's right
 * - Positive Y points up from the camera
 * - Positive Z points away from the camera (toward viewer in screen space)
 * 
 * @return 4x4 view transformation matrix
 */
glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

/**
 * Process keyboard input for camera movement
 * 
 * Implements frame-rate independent movement using delta time
 * Movement vectors are calculated relative to camera orientation:
 * - FORWARD/BACKWARD: move along camera's front vector
 * - LEFT/RIGHT: move along camera's right vector  
 * - UP/DOWN: move along world up vector (for flying camera)
 * 
 * Mathematical basis:
 * - Velocity = speed × deltaTime (ensures consistent movement regardless of framerate)
 * - New position = old position ± (direction vector × velocity)
 * 
 * @param direction Movement direction enum
 * @param deltaTime Time elapsed since last frame in seconds
 */
void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime; // Frame-independent velocity calculation
    
    switch (direction) {
        case CameraMovement::FORWARD:
            m_position += m_front * velocity;    // Move forward along camera front vector
            break;
        case CameraMovement::BACKWARD:
            m_position -= m_front * velocity;    // Move backward along camera front vector
            break;
        case CameraMovement::LEFT:
            m_position -= m_right * velocity;    // Strafe left along camera right vector
            break;
        case CameraMovement::RIGHT:
            m_position += m_right * velocity;    // Strafe right along camera right vector
            break;
        case CameraMovement::UP:
            m_position += m_worldUp * velocity;  // Move up along world up vector
            break;
        case CameraMovement::DOWN:
            m_position -= m_worldUp * velocity;  // Move down along world up vector
            break;
    }
}

/**
 * Process mouse movement for camera rotation (mouse look)
 * 
 * Converts mouse movement to camera rotation using Euler angles:
 * - Horizontal mouse movement affects yaw (rotation around Y-axis)
 * - Vertical mouse movement affects pitch (rotation around X-axis)
 * 
 * Mathematical approach:
 * - Mouse sensitivity controls rotation speed
 * - Pitch is typically clamped to prevent gimbal lock and disorientation
 * - Angles are accumulated over time for smooth rotation
 * 
 * Gimbal lock prevention:
 * - Pitch clamped to [-89°, +89°] to prevent camera flipping
 * - This keeps the camera from looking completely up or down
 * 
 * @param xOffset Horizontal mouse movement (positive = right)
 * @param yOffset Vertical mouse movement (positive = up)  
 * @param constrainPitch Whether to limit vertical rotation angle
 */
void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    // Apply sensitivity scaling to mouse input
    xOffset *= m_mouseSensitivity;
    yOffset *= m_mouseSensitivity;
    
    // Accumulate rotation angles
    m_yaw += xOffset;    // Horizontal rotation (left/right)
    m_pitch += yOffset;  // Vertical rotation (up/down)
    
    // Prevent gimbal lock by constraining pitch angle
    if (constrainPitch) {
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }
    
    // Recalculate camera orientation vectors from new angles
    updateCameraVectors();
}

/**
 * Process mouse scroll wheel for zoom control
 * 
 * Adjusts the camera's field of view (FOV) to simulate zoom:
 * - Smaller FOV = zoomed in (telephoto lens effect)
 * - Larger FOV = zoomed out (wide angle lens effect)
 * 
 * Mathematical relationship:
 * - FOV controls perspective projection matrix
 * - Smaller FOV increases magnification of distant objects
 * - Clamped to reasonable range [1°, 45°] for usability
 * 
 * @param yOffset Scroll wheel movement (positive = zoom in)
 */
void Camera::processMouseScroll(float yOffset) {
    m_zoom -= yOffset;  // Inverted: positive scroll zooms in (reduces FOV)
    m_zoom = std::clamp(m_zoom, 1.0f, 45.0f);  // Clamp to reasonable zoom range
}

/**
 * Calculate camera orientation vectors from Euler angles
 * 
 * Converts yaw and pitch angles to directional vectors using spherical coordinates:
 * 
 * Mathematical derivation:
 * Given yaw (θ) and pitch (φ) in degrees:
 * - front.x = cos(θ) × cos(φ)    [horizontal component of front vector]
 * - front.y = sin(φ)             [vertical component of front vector]  
 * - front.z = sin(θ) × cos(φ)    [depth component of front vector]
 * 
 * Then calculate perpendicular vectors:
 * - right = normalize(cross(front, worldUp))  [camera's right vector]
 * - up = normalize(cross(right, front))       [camera's up vector]
 * 
 * This creates an orthonormal basis (right-handed coordinate system)
 * where all three vectors are mutually perpendicular and unit length.
 */
void Camera::updateCameraVectors() {
    // Calculate front vector from spherical coordinates
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));  // X component
    front.y = sin(glm::radians(m_pitch));                             // Y component
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));  // Z component
    
    // Normalize and calculate perpendicular vectors using cross products
    m_front = glm::normalize(front);                                    // Forward direction
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));          // Right direction
    m_up = glm::normalize(glm::cross(m_right, m_front));               // Up direction
}