// ================================================================
// main.cpp — Entry Point & Game Loop (UPGRADED)
// ================================================================

#include "glfw_window.h"
#include "shader_utils.h"
#include "player.h"
#include "enemy.h"
#include "collision.h"
#include "hud.h"
#include "game_state.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

// ── Ground Quad ──────────────────────────────────────────────────
static unsigned int g_groundVAO = 0, g_groundVBO = 0;

void setupGround()
{
    float T = TERRAIN_SIZE, Y = -0.05f;
    float verts[] = {
        -T,Y, T,   T,Y, T,   T,Y,-T,
         T,Y,-T,  -T,Y,-T,  -T,Y, T,
    };
    glGenVertexArrays(1, &g_groundVAO); glGenBuffers(1, &g_groundVBO);
    glBindVertexArray(g_groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void drawGround(unsigned int shader,
                const glm::mat4& view, const glm::mat4& proj)
{
    glm::mat4 mvp = proj * view * glm::mat4(1.0f);
    glUseProgram(shader);
    setMVP(shader, mvp);
    setColor(shader, 0.22f, 0.45f, 0.20f);  // dark grass green
    glBindVertexArray(g_groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── Terrain boundary walls (visible edges of the play area) ──────
static unsigned int g_wallVAO = 0, g_wallVBO = 0;

void setupWalls()
{
    float T = TERRAIN_SIZE, H = 4.0f; // wall height
    // 4 walls as thin flat quads at terrain edge
    float verts[] = {
        // North wall (z = -T)
        -T, -0.05f,-T,   T, -0.05f,-T,   T,H,-T,
         T, H,     -T,  -T, H,     -T,  -T, -0.05f,-T,
        // South wall (z = +T)
        -T, -0.05f, T,   T,H, T,   T, -0.05f, T,
        -T, -0.05f, T,  -T,H, T,   T, H, T,
        // West wall (x = -T)
        -T, -0.05f,-T,  -T,H,-T,  -T, -0.05f, T,
        -T, H,-T,        -T, H, T, -T, -0.05f, T,
        // East wall (x = +T)
         T, -0.05f,-T,   T, -0.05f, T,  T,H,-T,
         T, H,-T,         T, -0.05f, T,  T, H, T,
    };
    glGenVertexArrays(1, &g_wallVAO); glGenBuffers(1, &g_wallVBO);
    glBindVertexArray(g_wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_wallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void drawWalls(unsigned int shader,
               const glm::mat4& view, const glm::mat4& proj)
{
    glm::mat4 mvp = proj * view * glm::mat4(1.0f);
    glUseProgram(shader);
    setMVP(shader, mvp);
    setColor(shader, 0.35f, 0.28f, 0.18f);  // earthy brown walls
    glBindVertexArray(g_wallVAO);
    glDrawArrays(GL_TRIANGLES, 0, 24);
    glBindVertexArray(0);
}

// ── main ─────────────────────────────────────────────────────────
int main()
{
    GLFWWindow       win(SCR_WIDTH, SCR_HEIGHT, "Drone Shooter [UPGRADED]");
    unsigned int     shader = createShaderProgram();
    Player           player;
    EnemyManager     enemies;
    CollisionDetector collision;
    HUD              hud;

    setupGround();
    setupWalls();

    GameState state        = GameState::START;
    float     wavePauseT   = 0.0f;  // time elapsed since WAVE_END began
    bool      bannerShown  = false;  // is wave banner currently visible

    std::cout << "\n=== DRONE SHOOTER (UPGRADED) ===\n"
              << "  WASD      Move     |  SHIFT  Sprint\n"
              << "  Mouse     Look     |  SPACE  Shoot\n"
              << "  R         Reload   |  ESC    Quit\n"
              << "  ENTER     Start / Restart\n"
              << "================================\n\n";

    while (!win.shouldClose())
    {
        float dt   = win.getDeltaTime();
        float time = static_cast<float>(glfwGetTime());

        // Sky colour shifts slightly warmer in later waves
        float waveHeat = std::min((enemies.waveNumber - 1) * 0.03f, 0.15f);
        glClearColor(0.53f - waveHeat, 0.81f - waveHeat, 0.98f - waveHeat*2, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ──────────────────────────────────────────────────────────
        if (state == GameState::START)
        {
            auto view = player.getViewMatrix();
            auto proj = player.getProjMatrix();
            drawGround(shader, view, proj);

            hud.drawStartScreen(shader, player.getOrthoMatrix(), time);

            if (glfwGetKey(win.handle, GLFW_KEY_ENTER) == GLFW_PRESS) {
                state = GameState::PLAYING;
                enemies.reset();
            }
        }

        // ──────────────────────────────────────────────────────────
        else if (state == GameState::PLAYING)
        {
            // ── Update ───────────────────────────────────────────
            player.processInput(win.handle, dt);
            player.processMouse(win.handle);
            player.updateTimers(dt);               // ammo reload, flash, etc.

            enemies.update(player.position, dt, time);

            int earned = collision.detect(player, enemies);
            (void)earned;  // score is added inside collision via player.score

            // Check wave complete
            if (enemies.waveComplete) {
                state      = GameState::WAVE_END;
                wavePauseT = 0.0f;
                bannerShown = true;
            }

            // Check game over
            if (player.health <= 0.0f)
                state = GameState::DEAD;

            // ── 3D Render ────────────────────────────────────────
            auto view  = player.getViewMatrix();
            auto proj  = player.getProjMatrix();
            auto ortho = player.getOrthoMatrix();

            drawGround(shader, view, proj);
            drawWalls(shader, view, proj);
            enemies.draw(shader, view, proj, ortho);  // draws drones + HP bars

            // ── 2D HUD ───────────────────────────────────────────
            glDisable(GL_DEPTH_TEST);

            // Danger ring (pulsing edge when enemy is close)
            float nearDist = collision.distToClosestEnemy(player, enemies);
            hud.drawDangerRing(shader, ortho, nearDist, time);

            player.drawGunBar   (shader);
            player.drawCrosshair(shader);
            player.drawMuzzleFlash(shader);

            hud.drawHealthBar (shader, ortho, player.health);
            hud.drawAmmoBar   (shader, ortho, player.ammo, MAX_AMMO, player.reloading);
            hud.drawScore     (shader, ortho, player.score);
            hud.drawMiniRadar (shader, ortho, player.position, enemies.enemies);

            glEnable(GL_DEPTH_TEST);
        }

        // ──────────────────────────────────────────────────────────
        else if (state == GameState::WAVE_END)
        {
            // Freeze gameplay, show wave banner
            auto view  = player.getViewMatrix();
            auto proj  = player.getProjMatrix();
            auto ortho = player.getOrthoMatrix();

            drawGround(shader, view, proj);
            drawWalls (shader, view, proj);
            // Draw remaining scenery
            enemies.draw(shader, view, proj, ortho);

            hud.drawWaveBanner(shader, ortho, enemies.waveNumber, wavePauseT);

            wavePauseT += dt;
            if (wavePauseT >= WAVE_PAUSE) {
                enemies.startNextWave();           // spawn next wave
                state      = GameState::PLAYING;
                bannerShown = false;
            }
        }

        // ──────────────────────────────────────────────────────────
        else if (state == GameState::DEAD)
        {
            auto view  = player.getViewMatrix();
            auto proj  = player.getProjMatrix();
            auto ortho = player.getOrthoMatrix();

            drawGround(shader, view, proj);
            hud.drawGameOverScreen(shader, ortho, player.score, player.kills, time);

            if (glfwGetKey(win.handle, GLFW_KEY_ENTER) == GLFW_PRESS) {
                player.reset();
                enemies.reset();
                state = GameState::PLAYING;
            }
        }

        win.swapAndPoll();
    }

    glDeleteVertexArrays(1, &g_groundVAO); glDeleteBuffers(1, &g_groundVBO);
    glDeleteVertexArrays(1, &g_wallVAO);   glDeleteBuffers(1, &g_wallVBO);
    glDeleteProgram(shader);

    std::cout << "Final score: " << player.score
              << "  Kills: " << player.kills
              << "  Wave reached: " << enemies.waveNumber << "\n";
    return 0;
}
