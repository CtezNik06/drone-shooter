#pragma once

// ================================================================
// MEMBER 5: hud.h  —  HUD & Game State
// ================================================================
// YOUR JOB:
//   • Draw the health bar (2 rectangles) in 2D screen space
//   • Draw kill markers (small squares — one per kill)
//   • Draw the START screen overlay (before game begins)
//   • Draw the GAME OVER screen overlay (when player dies)
//   • All drawing uses the orthographic projection from Player
//
// KEY CONCEPTS TO EXPLAIN (when presenting):
//   1. Orthographic Projection (2D rendering)
//      Unlike the 3D perspective projection, ortho maps pixel
//      coordinates (0..800, 0..600) directly to the screen with
//      no depth distortion.  Used for all UI elements.
//
//   2. HUD (Heads-Up Display)
//      The 2D overlay drawn on top of the 3D scene.
//      We draw 2D shapes AFTER the 3D scene using depth test OFF.
//
//   3. Game State Machine
//      The game is in exactly one state at a time:
//        START → PLAYING (press ENTER)
//        PLAYING → DEAD  (health reaches 0)
//        DEAD → PLAYING  (press ENTER to restart)
//
//   4. Alpha Blending (semi-transparent overlays)
//      We enable GL_BLEND so our overlay quads can be translucent.
//      Formula: output = src_alpha * src + (1-src_alpha) * dst
// ================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "game_state.h"

class HUD {
public:
    HUD();
    ~HUD();

    // Draw the health bar at the top-left of the screen.
    // health = current health (0..100).  Below 30 it turns red.
    void drawHealthBar(unsigned int shader,
                       const glm::mat4& ortho,
                       float health);

    // Draw one small colored square for each kill (tally markers).
    void drawKillMarkers(unsigned int shader,
                         const glm::mat4& ortho,
                         int kills);

    // START SCREEN: semi-transparent dark overlay + pulsing green bar
    // (player should press ENTER to start)
    void drawStartScreen(unsigned int shader,
                         const glm::mat4& ortho,
                         float time);

    // GAME OVER: semi-transparent red overlay + white bar showing score
    void drawGameOverScreen(unsigned int shader,
                            const glm::mat4& ortho,
                            int kills,
                            float time);

private:
    // VAO/VBO for a single reusable rectangle (we transform it each use)
    unsigned int m_quadVAO, m_quadVBO;

    void setupQuad();

    // Draws a filled rectangle at (x,y) with width w and height h.
    // Uses the orthographic projection matrix `ortho`.
    // r, g, b, a = color including alpha
    void drawRect(unsigned int shader,
                  const glm::mat4& ortho,
                  float x, float y, float w, float h,
                  float r, float g, float b, float a = 1.0f);
};
