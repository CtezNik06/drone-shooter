// ================================================================
// MEMBER 5 (UPGRADED): hud.cpp
// New: ammo bar, score, mini-radar, danger ring, wave banner,
//      segmented health, improved start/game-over screens
// ================================================================

#include "hud.h"
#include "shader_utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

HUD::HUD()  { setupQuad(); }
HUD::~HUD() { glDeleteVertexArrays(1, &m_quadVAO); glDeleteBuffers(1, &m_quadVBO); }

// ── Unit quad (reused for every rectangle) ───────────────────────
void HUD::setupQuad()
{
    float verts[] = {
        0.f,0.f,0.f,  1.f,0.f,0.f,  1.f,1.f,0.f,
        1.f,1.f,0.f,  0.f,1.f,0.f,  0.f,0.f,0.f,
    };
    glGenVertexArrays(1, &m_quadVAO); glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ── Core drawRect helper ─────────────────────────────────────────
void HUD::drawRect(unsigned int shader, const glm::mat4& ortho,
                   float x, float y, float w, float h,
                   float r, float g, float b, float a)
{
    glm::mat4 model = glm::scale(
        glm::translate(glm::mat4(1.f), glm::vec3(x, y, 0.f)),
        glm::vec3(w, h, 1.f));
    glUseProgram(shader);
    setMVP(shader, ortho * model);
    setColor(shader, r, g, b, a);
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── Hollow border: 4 thin rects forming a rectangle outline ──────
void HUD::drawBorder(unsigned int shader, const glm::mat4& ortho,
                     float x, float y, float w, float h, float t,
                     float r, float g, float b, float a)
{
    drawRect(shader, ortho, x,       y,       w, t, r,g,b,a);  // bottom
    drawRect(shader, ortho, x,       y+h-t,   w, t, r,g,b,a);  // top
    drawRect(shader, ortho, x,       y,       t, h, r,g,b,a);  // left
    drawRect(shader, ortho, x+w-t,   y,       t, h, r,g,b,a);  // right
}

// ── Segmented Health Bar (NEW) ────────────────────────────────────
// Divided into 5 segments (each = 20 HP).  Each segment is a
// separate rectangle — full segments are bright, empty are dark.
void HUD::drawHealthBar(unsigned int shader,
                        const glm::mat4& ortho, float health)
{
    glDisable(GL_DEPTH_TEST);

    const int   SEG   = 5;
    const float segW  = 36.0f, segH = 18.0f, gap = 3.0f;
    const float startX = 10.0f, startY = SCR_HEIGHT - 32.0f;
    const float hpPerSeg = PLAYER_MAX_HP / SEG;

    for (int i = 0; i < SEG; ++i) {
        float x        = startX + i * (segW + gap);
        float segStart = i * hpPerSeg;
        float fill     = std::clamp((health - segStart) / hpPerSeg, 0.0f, 1.0f);

        // Background (dark)
        drawRect(shader, ortho, x, startY, segW, segH, 0.2f, 0.0f, 0.0f);

        // Foreground — colour shifts red→orange→green as fill increases
        float gr = (fill > 0.5f) ? 0.9f : fill * 1.8f;
        float rd = (fill > 0.5f) ? (1.0f - fill) * 2.0f : 1.0f;
        drawRect(shader, ortho, x, startY, segW * fill, segH, rd, gr, 0.0f);

        // Segment border
        drawBorder(shader, ortho, x, startY, segW, segH, 1.5f, 0.6f, 0.6f, 0.6f);
    }

    glEnable(GL_DEPTH_TEST);
}

// ── Ammo Bar (NEW) ───────────────────────────────────────────────
// Shows MAX_AMMO slots.  Full = white, empty = dark, reloading = all orange.
void HUD::drawAmmoBar(unsigned int shader, const glm::mat4& ortho,
                      int ammo, int maxAmmo, bool reloading)
{
    glDisable(GL_DEPTH_TEST);

    const float slotW = 12.0f, slotH = 18.0f, gap = 3.0f;
    const float startX = 10.0f, startY = SCR_HEIGHT - 58.0f;

    for (int i = 0; i < maxAmmo; ++i) {
        float x = startX + i * (slotW + gap);

        if (reloading) {
            // Orange pulsing during reload
            drawRect(shader, ortho, x, startY, slotW, slotH, 0.9f, 0.4f, 0.0f);
        } else if (i < ammo) {
            drawRect(shader, ortho, x, startY, slotW, slotH, 0.9f, 0.9f, 0.9f); // white = loaded
        } else {
            drawRect(shader, ortho, x, startY, slotW, slotH, 0.2f, 0.2f, 0.2f); // dark = empty
        }
        drawBorder(shader, ortho, x, startY, slotW, slotH, 1.0f, 0.5f, 0.5f, 0.5f);
    }

    glEnable(GL_DEPTH_TEST);
}

// ── Score Display (NEW) ──────────────────────────────────────────
// A white bar at the top centre whose width represents the score.
// Max bar width reached at 5000 points.
void HUD::drawScore(unsigned int shader, const glm::mat4& ortho, int score)
{
    glDisable(GL_DEPTH_TEST);

    const float barW  = 200.0f, barH = 10.0f;
    const float startX = (SCR_WIDTH - barW) / 2.0f;
    const float startY = SCR_HEIGHT - 18.0f;
    const float maxScore = 5000.0f;

    float frac = std::min(score / maxScore, 1.0f);

    drawRect(shader, ortho, startX, startY, barW, barH, 0.15f, 0.15f, 0.15f); // bg
    drawRect(shader, ortho, startX, startY, barW * frac, barH, 0.9f, 0.85f, 0.2f); // gold bar
    drawBorder(shader, ortho, startX, startY, barW, barH, 1.5f, 0.6f, 0.6f, 0.0f);

    glEnable(GL_DEPTH_TEST);
}

// ── Mini-Radar (NEW) ─────────────────────────────────────────────
// A top-right 120×120 px square that acts as a top-down map.
// Player = white dot at centre.  Enemies = red/orange dots.
// The radar range covers TERRAIN_SIZE * 2 units of world space.
//
// KEY CONCEPT: Converting world-space position to radar-space:
//   relativePos = enemyPos - playerPos          ← offset from player
//   radarX = radarCentreX + relativePos.x / range * radarRadius
//   radarY = radarCentreY + relativePos.z / range * radarRadius
void HUD::drawMiniRadar(unsigned int shader, const glm::mat4& ortho,
                         const glm::vec3& playerPos,
                         const std::vector<Enemy>& enemies)
{
    glDisable(GL_DEPTH_TEST);

    const float SIZE   = 120.0f;
    const float MARGIN = 10.0f;
    const float rx     = SCR_WIDTH  - SIZE - MARGIN;  // top-right
    const float ry     = SCR_HEIGHT - SIZE - MARGIN;
    const float radius = SIZE * 0.5f;
    const float range  = TERRAIN_SIZE * 1.8f;         // world units shown

    // Background
    drawRect(shader, ortho, rx, ry, SIZE, SIZE, 0.04f, 0.04f, 0.08f, 0.8f);

    // Grid lines (cross through centre)
    float cx = rx + radius, cy = ry + radius;
    drawRect(shader, ortho, cx - 0.5f, ry, 1.0f, SIZE, 0.2f, 0.3f, 0.2f, 0.5f); // vertical
    drawRect(shader, ortho, rx, cy - 0.5f, SIZE, 1.0f, 0.2f, 0.3f, 0.2f, 0.5f); // horizontal

    // Enemy dots
    for (auto& e : enemies) {
        if (!e.active) continue;

        glm::vec3 rel = e.position - playerPos;
        float ex = cx + rel.x / range * radius;
        float ey = cy + rel.z / range * radius;

        // Clamp to radar bounds
        ex = std::clamp(ex, rx + 3.0f, rx + SIZE - 3.0f);
        ey = std::clamp(ey, ry + 3.0f, ry + SIZE - 3.0f);

        float dotSize = (e.type == EnemyType::HEAVY) ? 8.0f : 5.0f;
        float er = (e.type == EnemyType::HEAVY) ? 1.0f : 1.0f;
        float eg = (e.type == EnemyType::HEAVY) ? 0.2f : 0.6f;
        float eb = 0.1f;

        drawRect(shader, ortho, ex - dotSize/2, ey - dotSize/2,
                 dotSize, dotSize, er, eg, eb);
    }

    // Player dot (bright white at centre)
    drawRect(shader, ortho, cx - 4.0f, cy - 4.0f, 8.0f, 8.0f, 1.0f, 1.0f, 1.0f);

    // Radar border
    drawBorder(shader, ortho, rx, ry, SIZE, SIZE, 2.0f, 0.2f, 0.8f, 0.2f);

    glEnable(GL_DEPTH_TEST);
}

// ── Danger Ring (NEW) ─────────────────────────────────────────────
// When the nearest enemy is within 8 units, the screen edge pulses red.
// The closer the enemy, the brighter and faster the pulse.
void HUD::drawDangerRing(unsigned int shader, const glm::mat4& ortho,
                          float dangerDist, float time)
{
    glDisable(GL_DEPTH_TEST);

    const float triggerDist = 8.0f;
    if (dangerDist > triggerDist) { glEnable(GL_DEPTH_TEST); return; }

    // Intensity 0..1: stronger when closer
    float intensity = 1.0f - (dangerDist / triggerDist);
    // Pulse frequency also increases when closer
    float pulse = 0.5f + 0.5f * sin(time * (6.0f + intensity * 8.0f));
    float alpha = intensity * pulse * 0.55f;

    float t = 18.0f;  // border thickness in pixels
    drawRect(shader, ortho, 0, 0, (float)SCR_WIDTH, t, 1,0,0,alpha);              // bottom
    drawRect(shader, ortho, 0, SCR_HEIGHT-t, (float)SCR_WIDTH, t, 1,0,0,alpha);   // top
    drawRect(shader, ortho, 0, 0, t, (float)SCR_HEIGHT, 1,0,0,alpha);             // left
    drawRect(shader, ortho, SCR_WIDTH-t, 0, t, (float)SCR_HEIGHT, 1,0,0,alpha);   // right

    glEnable(GL_DEPTH_TEST);
}

// ── Wave Banner (NEW) ─────────────────────────────────────────────
// Displays a full-width banner for WAVE_PAUSE seconds when a new
// wave starts.  time = seconds elapsed since the banner appeared.
void HUD::drawWaveBanner(unsigned int shader, const glm::mat4& ortho,
                          int waveNumber, float time)
{
    glDisable(GL_DEPTH_TEST);

    // Banner fades in during first 0.4s, stays, then fades out in last 0.4s
    float alpha = 1.0f;
    if (time < 0.4f)            alpha = time / 0.4f;
    if (time > WAVE_PAUSE - 0.4f) alpha = (WAVE_PAUSE - time) / 0.4f;
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    // Background stripe
    drawRect(shader, ortho, 0, SCR_HEIGHT * 0.45f, (float)SCR_WIDTH, SCR_HEIGHT * 0.1f,
             0.0f, 0.0f, 0.0f, 0.7f * alpha);

    // Coloured accent bar (colour depends on wave)
    float wr = 0.1f + (waveNumber * 0.15f);   // gets redder each wave
    float wg = 0.9f - (waveNumber * 0.15f);
    wr = std::clamp(wr, 0.1f, 1.0f);
    wg = std::clamp(wg, 0.0f, 0.9f);

    float barY = SCR_HEIGHT * 0.45f;
    drawRect(shader, ortho, 0, barY, (float)SCR_WIDTH, 4.0f, wr, wg, 0.0f, alpha);
    drawRect(shader, ortho, 0, barY + SCR_HEIGHT*0.1f - 4, (float)SCR_WIDTH, 4.0f, wr, wg, 0.0f, alpha);

    // Wave-number indicator blocks (N small squares = wave N)
    float blockSize = 20.0f, gap = 6.0f;
    float totalW = waveNumber * (blockSize + gap) - gap;
    float bx = (SCR_WIDTH - totalW) / 2.0f;
    float by  = SCR_HEIGHT * 0.48f;

    for (int i = 0; i < waveNumber; ++i) {
        float pulse = 0.7f + 0.3f * sin(time * 4.0f + i * 1.2f);
        drawRect(shader, ortho, bx + i*(blockSize+gap), by,
                 blockSize, blockSize, wr * pulse, wg * pulse, 0.0f, alpha);
    }

    glEnable(GL_DEPTH_TEST);
}

// ── Start Screen ─────────────────────────────────────────────────
void HUD::drawStartScreen(unsigned int shader,
                           const glm::mat4& ortho, float time)
{
    glDisable(GL_DEPTH_TEST);

    // Dark overlay
    drawRect(shader, ortho, 0, 0, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0,0,0, 0.6f);

    // Title bar (cyan)
    drawRect(shader, ortho, 50, 390, 700, 55, 0.0f, 0.8f, 0.8f);
    drawRect(shader, ortho, 55, 396, 690, 43, 0.0f, 0.3f, 0.3f);

    // Pulsing ENTER bar (green)
    float p = 0.5f + 0.5f * sin(time * 3.0f);
    drawRect(shader, ortho, 200, 270, 400, 28, 0, p * 0.9f, 0);

    // WASD key layout diagram
    float ky = 175.0f, kx = 350.0f, ks = 26.0f;
    drawRect(shader, ortho, kx, ky+ks+4, ks, ks, 0.8f, 0.8f, 0.8f); // W
    drawRect(shader, ortho, kx-ks-4, ky, ks, ks, 0.8f, 0.8f, 0.8f); // A
    drawRect(shader, ortho, kx, ky, ks, ks, 0.9f, 0.9f, 0.5f);       // S (highlight)
    drawRect(shader, ortho, kx+ks+4, ky, ks, ks, 0.8f, 0.8f, 0.8f); // D

    // SPACE bar
    drawRect(shader, ortho, 270, 130, 260, 20, 0.6f, 0.6f, 0.6f);

    // SHIFT key (sprint indicator)
    drawRect(shader, ortho, 270, 102, 120, 18, 0.5f, 0.5f, 0.7f);

    // R key (reload indicator)
    drawRect(shader, ortho, 420, 102, 40, 18, 0.7f, 0.4f, 0.1f);

    glEnable(GL_DEPTH_TEST);
}

// ── Game Over Screen ─────────────────────────────────────────────
void HUD::drawGameOverScreen(unsigned int shader, const glm::mat4& ortho,
                              int score, int kills, float time)
{
    glDisable(GL_DEPTH_TEST);

    // Red overlay
    drawRect(shader, ortho, 0, 0, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.4f, 0.0f, 0.0f, 0.65f);

    // Dark title band
    drawRect(shader, ortho, 0, 360, (float)SCR_WIDTH, 75, 0.12f, 0.0f, 0.0f);
    drawRect(shader, ortho, 0, 365, (float)SCR_WIDTH, 4, 1.0f, 0.2f, 0.2f);

    // Score bar (gold, width = score / 5000)
    float frac = std::min(score / 5000.0f, 1.0f);
    float bx = 100.0f;
    drawRect(shader, ortho, bx, 280, 600, 20, 0.2f, 0.1f, 0.0f);           // bg
    drawRect(shader, ortho, bx, 280, 600 * frac, 20, 0.95f, 0.8f, 0.15f);  // gold

    // Kill counter: green squares
    for (int i = 0; i < std::min(kills, 30); ++i)
        drawRect(shader, ortho, 100 + i * 14.0f, 250, 10, 12, 0.2f, 0.9f, 0.2f);

    // Pulsing restart bar
    float p = 0.5f + 0.5f * sin(time * 4.0f);
    drawRect(shader, ortho, 230, 195, 340, 26, 0.0f, p * 0.85f, 0.0f);

    glEnable(GL_DEPTH_TEST);
}
