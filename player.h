#pragma once

// ================================================================
// MEMBER 2 (UPGRADED): player.h
// NEW FEATURES ADDED:
//   • Shot cooldown  — can't spam shoot, must wait SHOT_COOLDOWN s
//   • Ammo system    — 10 bullets, auto-reload when empty
//   • Sprint (SHIFT) — faster movement, stronger bob
//   • Muzzle flash   — brief bright flash at crosshair when firing
//   • Score tracking — total points earned
//   • Camera bob     — smooth head-bob while walking/sprinting
// ================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Player {
public:
    // ── Position & orientation ───────────────────────────────────
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;

    // ── Game stats ───────────────────────────────────────────────
    float health;
    int   kills;
    int   score;         // NEW: total score
    bool  shot;          // true ONLY on the frame player fires
    float range;

    // ── Ammo system (NEW) ────────────────────────────────────────
    int   ammo;          // current bullets remaining
    bool  reloading;     // true while reloading
    float reloadTimer;   // counts down from RELOAD_TIME

    // ── Shot cooldown (NEW) ──────────────────────────────────────
    float shotCooldown;  // counts down to 0 between shots

    // ── Muzzle flash (NEW) ──────────────────────────────────────
    bool  muzzleFlash;   // true = draw flash this frame
    float flashTimer;    // how long flash lasts

    // ── Movement state ───────────────────────────────────────────
    bool  isSprinting;   // true when SHIFT held
    bool  isMoving;      // true when WASD pressed this frame

    // ── Bounding box (used by Member 4 — collision) ─────────────
    glm::vec3 bboxMin, bboxMax;

    // ── Constructor / Destructor ─────────────────────────────────
    Player();
    ~Player();

    // Process WASD movement + SPACE shoot + SHIFT sprint each frame.
    void processInput(GLFWwindow* win, float dt);

    // Update camera direction from mouse.
    void processMouse(GLFWwindow* win);

    // Called by collision when laser hits player.
    void takeDamage(float amount);

    // Rebuild AABB around current position.
    void updateBoundingBox();

    // ── Matrix getters ───────────────────────────────────────────
    glm::mat4 getViewMatrix()  const;
    glm::mat4 getProjMatrix()  const;
    glm::mat4 getOrthoMatrix() const;

    // ── 2D Draw calls (render after all 3D drawing) ──────────────
    void drawCrosshair  (unsigned int shader);   // + crosshair
    void drawGunBar     (unsigned int shader);   // dark gun rect
    void drawMuzzleFlash(unsigned int shader);   // NEW: yellow burst

    // Called each frame to update timers (ammo reload, flash, bob)
    void updateTimers(float dt);

    // Reset everything for a new game.
    void reset();

private:
    float m_yaw, m_pitch;
    float m_lastX, m_lastY;
    bool  m_firstMouse;
    float m_bobAngle;    // NEW: head bob oscillation angle

    unsigned int m_crosshairVAO, m_crosshairVBO;
    unsigned int m_gunBarVAO,    m_gunBarVBO;
    unsigned int m_flashVAO,     m_flashVBO;    // NEW: muzzle flash quad

    void updateVectors();
    void setupCrosshairGeometry();
    void setupGunBarGeometry();
    void setupFlashGeometry();   // NEW
};
