#pragma once

// ================================================================
// MEMBER 5 (UPGRADED): hud.h
// NEW FEATURES:
//   • Ammo bar       — shows remaining bullets (10 slots)
//   • Score display  — animated score bar
//   • Wave banner    — "WAVE N" announcement at wave start
//   • Danger ring    — pulsing red outline when enemy is close
//   • Mini-radar     — top-right map showing enemy positions
//   • Segmented HP   — health bar split into 5 segments
// ================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "game_state.h"
#include "enemy.h"    // needed for mini-radar (reads enemy positions)
#include <vector>

class HUD {
public:
    HUD();
    ~HUD();

    // ── In-game HUD elements ─────────────────────────────────────

    // Segmented health bar (5 segments, each = 20 HP)
    void drawHealthBar(unsigned int shader,
                       const glm::mat4& ortho, float health);

    // Ammo slots: white = loaded, dark = empty, orange = reloading
    void drawAmmoBar(unsigned int shader,
                     const glm::mat4& ortho,
                     int ammo, int maxAmmo, bool reloading);

    // Score display: a white bar whose width grows with score
    void drawScore(unsigned int shader,
                   const glm::mat4& ortho, int score);

    // Mini-radar (top-right): player dot at centre, enemies as dots
    void drawMiniRadar(unsigned int shader,
                       const glm::mat4& ortho,
                       const glm::vec3& playerPos,
                       const std::vector<Enemy>& enemies);

    // Pulsing red border when an enemy is within dangerDist units
    void drawDangerRing(unsigned int shader,
                        const glm::mat4& ortho,
                        float dangerDist,   // distance to closest enemy
                        float time);

    // Wave announcement banner (shown for wavePause seconds)
    void drawWaveBanner(unsigned int shader,
                        const glm::mat4& ortho,
                        int waveNumber, float time);

    // ── Full-screen overlay screens ──────────────────────────────
    void drawStartScreen   (unsigned int shader, const glm::mat4& ortho, float time);
    void drawGameOverScreen(unsigned int shader, const glm::mat4& ortho, int score, int kills, float time);

private:
    unsigned int m_quadVAO, m_quadVBO;
    void setupQuad();

    // Core primitive: draw a filled rectangle in 2D pixel space
    void drawRect(unsigned int shader, const glm::mat4& ortho,
                  float x, float y, float w, float h,
                  float r, float g, float b, float a = 1.0f);

    // Draw a hollow rectangle border (4 thin rects)
    void drawBorder(unsigned int shader, const glm::mat4& ortho,
                    float x, float y, float w, float h, float thickness,
                    float r, float g, float b, float a = 1.0f);
};
