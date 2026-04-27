#pragma once

// ================================================================
// MEMBER 2: player.h  —  Player (Camera, Movement, Crosshair)
// ================================================================
// YOUR JOB:
//   • Track where the player is and which way they face
//   • Move in response to WASD + detect SPACE (shoot)
//   • Rotate the camera when the mouse moves (mouse-look)
//   • Compute VIEW and PROJECTION matrices for ALL 3D rendering
//   • Draw the crosshair (two lines) and a simple gun rectangle
//
// KEY CONCEPTS TO EXPLAIN (when presenting):
//   1. Euler Angles (Yaw & Pitch) — two angles that describe where
//      the camera is looking.  Yaw = left/right, Pitch = up/down.
//      We compute the front vector from these every frame.
//
//   2. glm::lookAt(eye, target, up)  — VIEW matrix.
//      Shifts the whole world so the camera is at the origin looking
//      down -Z.  Every object's position is relative to this.
//
//   3. glm::perspective(fov, aspect, near, far)  — PROJECTION matrix.
//      Applies perspective divide: distant things look smaller.
//      Combined with view: MVP = Projection × View × Model.
//
//   4. glm::ortho(l, r, b, t)  — ORTHOGRAPHIC projection for HUD.
//      No perspective; pixel coords map 1-to-1 to screen coords.
//      Used for drawing 2D elements (crosshair, health bar).
// ================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Player {
public:
    // ── World-space position and orientation ──
    glm::vec3 position;  // Where the player (camera) is
    glm::vec3 front;     // Unit vector: direction the player looks
    glm::vec3 right;     // Unit vector: player's right side
    glm::vec3 up;        // Unit vector: local up

    // ── Game state ──
    float health;        // 0 to 100
    int   kills;         // How many enemies have been shot down
    bool  shot;          // True ONLY on the frame the player fires
    float range;         // Max bullet range (units)

    // ── Bounding box (used by collision) ──
    glm::vec3 bboxMin, bboxMax;

    // Constructor / Destructor
    Player();
    ~Player();

    // Process WASD (movement) and SPACE (shoot) each frame.
    // win = the GLFW window handle   dt = delta time in seconds
    void processInput(GLFWwindow* win, float dt);

    // Update camera direction from mouse movement.
    void processMouse(GLFWwindow* win);

    // Called by CollisionDetector when an enemy laser hits the player.
    void takeDamage(float amount);

    // Build the player's bounding box around current position.
    void updateBoundingBox();

    // ── Matrix getters used by ALL rendering code ──
    glm::mat4 getViewMatrix()  const;  // 3D view  (glm::lookAt)
    glm::mat4 getProjMatrix()  const;  // 3D perspective
    glm::mat4 getOrthoMatrix() const;  // 2D orthographic (HUD)

    // Draw the crosshair (two intersecting lines at screen center).
    void drawCrosshair(unsigned int shader);

    // Draw a dark gun bar rectangle at the bottom of the screen.
    void drawGunBar(unsigned int shader);

    // Reset all values for a new game.
    void reset();

private:
    float m_yaw;        // Horizontal look angle (degrees)
    float m_pitch;      // Vertical   look angle (degrees)
    float m_lastX;      // Previous mouse X (pixels)
    float m_lastY;      // Previous mouse Y (pixels)
    bool  m_firstMouse; // Skip the giant jump on the first mouse event

    // GPU objects for crosshair and gun bar geometry
    unsigned int m_crosshairVAO, m_crosshairVBO;
    unsigned int m_gunBarVAO,    m_gunBarVBO;

    // Recompute front/right/up from current yaw and pitch.
    void updateVectors();

    // Upload geometry to VAO/VBO (called once in constructor).
    void setupCrosshairGeometry();
    void setupGunBarGeometry();
};
