// ================================================================
// MEMBER 5: hud.cpp  —  HUD & Game State
// ================================================================

#include "hud.h"
#include "shader_utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>  // std::min
#include <cmath>      // sin

// ── Constructor ─────────────────────────────────────────────────
HUD::HUD()
{
    setupQuad();
}

HUD::~HUD()
{
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers     (1, &m_quadVBO);
}

// ── Set up a unit quad (1×1 rectangle) centred at origin ────────
// We scale and translate it in drawRect() to position each element.
void HUD::setupQuad()
{
    // A quad made of two triangles (6 vertices, no index buffer)
    // Vertices are in [0..1, 0..1] space; we scale in drawRect().
    float verts[] = {
        0.0f, 0.0f, 0.0f,    // bottom-left
        1.0f, 0.0f, 0.0f,    // bottom-right
        1.0f, 1.0f, 0.0f,    // top-right

        1.0f, 1.0f, 0.0f,    // top-right
        0.0f, 1.0f, 0.0f,    // top-left
        0.0f, 0.0f, 0.0f,    // bottom-left
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers     (1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ── Core helper: draw a filled rectangle using the unit quad ─────
// We build a MVP = ortho × scale × translate for each rect.
//
// ortho  : orthographic projection from Player::getOrthoMatrix()
// x, y   : bottom-left corner in pixels (0..SCR_WIDTH, 0..SCR_HEIGHT)
// w, h   : width and height in pixels
// r,g,b,a: colour including transparency (a < 1 = translucent)
void HUD::drawRect(unsigned int shader,
                   const glm::mat4& ortho,
                   float x, float y, float w, float h,
                   float r, float g, float b, float a)
{
    // Build model matrix: translate to (x,y) then scale to (w,h)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));  // move
    model = glm::scale    (model, glm::vec3(w, h, 1.0f));  // size

    // For HUD, View is identity (no camera transform)
    glm::mat4 mvp = ortho * model;

    glUseProgram(shader);
    setMVP  (shader, mvp);
    setColor(shader, r, g, b, a);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── Health Bar ───────────────────────────────────────────────────
// Location: top-left area.  Two rectangles stacked:
//   1. Gray background bar  (200 px wide)
//   2. Coloured foreground  (width proportional to health %)
void HUD::drawHealthBar(unsigned int shader,
                        const glm::mat4& ortho,
                        float health)
{
    // Turn off depth test so HUD draws on top of all 3D geometry.
    glDisable(GL_DEPTH_TEST);

    float barX = 10.0f;                    // left edge
    float barY = SCR_HEIGHT - 30.0f;       // top area (30 px from top)
    float barW = 200.0f;                   // full bar width in pixels
    float barH = 18.0f;                    // bar height in pixels

    // ── Background (dark gray) ──────────────────────────────────
    drawRect(shader, ortho, barX, barY, barW, barH,
             0.25f, 0.25f, 0.25f);

    // ── Foreground (health fraction × full width) ────────────────
    float fraction = health / PLAYER_MAX_HP;   // 0.0 to 1.0
    float fgW      = barW * fraction;

    // Color: green when healthy, red when critical
    float red  = 1.0f;
    float green = 0.0f;
    if (health > 60.0f) { red = 0.0f; green = 0.8f; }       // green
    else if (health > 30.0f) { red = 1.0f; green = 0.6f; }  // orange

    drawRect(shader, ortho, barX, barY, fgW, barH, red, green, 0.0f);

    // Re-enable depth test for the next 3D frame
    glEnable(GL_DEPTH_TEST);
}

// ── Kill Markers ─────────────────────────────────────────────────
// Draw one small green square per kill.  When >20 kills, show "20+"
// worth of squares to avoid going off-screen.
void HUD::drawKillMarkers(unsigned int shader,
                           const glm::mat4& ortho,
                           int kills)
{
    glDisable(GL_DEPTH_TEST);

    float startX = 10.0f;
    float markerY = SCR_HEIGHT - 55.0f;  // just below the health bar
    float size    = 8.0f;
    float gap     = 3.0f;

    int display = std::min(kills, 30);
    for (int i = 0; i < display; ++i) {
        float x = startX + i * (size + gap);
        drawRect(shader, ortho, x, markerY, size, size,
                 0.2f, 0.9f, 0.2f);  // bright green squares
    }

    glEnable(GL_DEPTH_TEST);
}

// ── Start Screen ─────────────────────────────────────────────────
// Shows a semi-transparent dark overlay and a pulsing green bar
// to signal "waiting for ENTER".
void HUD::drawStartScreen(unsigned int shader,
                           const glm::mat4& ortho,
                           float time)
{
    glDisable(GL_DEPTH_TEST);

    // Dark translucent overlay covering the whole screen
    drawRect(shader, ortho,
             0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT,
             0.0f, 0.0f, 0.0f, 0.6f);  // black at 60% opacity

    // Title bar: a wide cyan rectangle at the top third
    drawRect(shader, ortho,
             60.0f, 380.0f, 680.0f, 50.0f,
             0.0f, 0.8f, 0.8f);  // cyan "title" bar

    // Inner darker stripe on title
    drawRect(shader, ortho,
             70.0f, 390.0f, 660.0f, 30.0f,
             0.0f, 0.3f, 0.3f);

    // Pulsing green "PRESS ENTER" indicator (sine wave 0..1)
    float pulse = 0.5f + 0.5f * sin(time * 3.0f);  // oscillates 0..1
    drawRect(shader, ortho,
             200.0f, 260.0f, 400.0f, 30.0f,
             0.0f, pulse, 0.0f);  // pulsing green bar

    // WASD instruction markers (small white squares in a cross pattern)
    float ky = 180.0f;
    float kx = 360.0f;
    float ks = 24.0f;
    // W (top)
    drawRect(shader, ortho, kx, ky + ks + 4, ks, ks, 0.8f, 0.8f, 0.8f);
    // A (left)
    drawRect(shader, ortho, kx - ks - 4, ky, ks, ks, 0.8f, 0.8f, 0.8f);
    // S (centre)
    drawRect(shader, ortho, kx, ky, ks, ks, 0.8f, 0.8f, 0.8f);
    // D (right)
    drawRect(shader, ortho, kx + ks + 4, ky, ks, ks, 0.8f, 0.8f, 0.8f);

    // SPACE indicator (long horizontal bar)
    drawRect(shader, ortho, 280.0f, 135.0f, 240.0f, 22.0f, 0.6f, 0.6f, 0.6f);

    glEnable(GL_DEPTH_TEST);
}

// ── Game Over Screen ─────────────────────────────────────────────
// Red translucent overlay + a white bar whose width shows the score.
void HUD::drawGameOverScreen(unsigned int shader,
                              const glm::mat4& ortho,
                              int kills,
                              float time)
{
    glDisable(GL_DEPTH_TEST);

    // Red translucent overlay
    drawRect(shader, ortho,
             0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT,
             0.5f, 0.0f, 0.0f, 0.6f);  // dark red at 60% opacity

    // "GAME OVER" title bar — dark band
    drawRect(shader, ortho,
             0.0f, 360.0f, (float)SCR_WIDTH, 70.0f,
             0.15f, 0.0f, 0.0f);  // very dark red band

    // Red accent line on the band
    drawRect(shader, ortho,
             0.0f, 365.0f, (float)SCR_WIDTH, 5.0f,
             1.0f, 0.2f, 0.2f);

    // Score bar: white bar at centre, width = (kills / 30) × 600 px
    float scoreW = std::min(kills * 20.0f, 600.0f);
    float scoreX = (SCR_WIDTH - scoreW) / 2.0f;
    drawRect(shader, ortho,
             scoreX, 270.0f, scoreW, 20.0f,
             1.0f, 1.0f, 1.0f);   // white bar = score visualization

    // Pulsing green "PRESS ENTER to restart" indicator
    float pulse = 0.5f + 0.5f * sin(time * 4.0f);
    drawRect(shader, ortho,
             250.0f, 200.0f, 300.0f, 24.0f,
             0.0f, pulse * 0.8f, 0.0f);

    glEnable(GL_DEPTH_TEST);
}
