// ================================================================
// main.cpp  —  Entry Point & Game Loop
// This file assembles all 5 components into a working game.
// ================================================================
//  FLOW:
//    1. Member 1 (GLFWWindow)         → create window + load OpenGL
//    2. Compile shader                → shared GPU program
//    3. Member 2 (Player)             → camera, movement
//    4. Member 3 (EnemyManager)       → drones
//    5. Member 4 (CollisionDetector)  → hit detection
//    6. Member 5 (HUD)                → overlay UI
//    7. Ground quad (local setup)     → simple flat floor
//    8. GAME LOOP: input → update → draw → swap
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

// ── Ground Quad setup (simple flat floor) ───────────────────────
// We set this up once in main because it belongs to no single class.
static unsigned int g_groundVAO = 0;
static unsigned int g_groundVBO = 0;

void setupGround()
{
    float T = TERRAIN_SIZE;   // half-size of terrain
    float groundY = -0.05f;   // just below ground level

    // Six vertices = two triangles = one flat rectangle (XZ plane)
    float verts[] = {
        -T, groundY,  T,   // front-left
         T, groundY,  T,   // front-right
         T, groundY, -T,   // back-right

         T, groundY, -T,   // back-right
        -T, groundY, -T,   // back-left
        -T, groundY,  T,   // front-left
    };

    glGenVertexArrays(1, &g_groundVAO);
    glGenBuffers     (1, &g_groundVBO);
    glBindVertexArray(g_groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void drawGround(unsigned int shader,
                const glm::mat4& view,
                const glm::mat4& proj)
{
    // Model matrix = identity (ground is already in world space)
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp   = proj * view * model;

    glUseProgram(shader);
    setMVP  (shader, mvp);
    setColor(shader, 0.28f, 0.52f, 0.26f);  // dark grass green

    glBindVertexArray(g_groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── main ─────────────────────────────────────────────────────────
int main()
{
    // ── 1. Create window (Member 1) ───────────────────────────────
    GLFWWindow win(SCR_WIDTH, SCR_HEIGHT, "Drone Shooter  [SIMPLE]");

    // ── 2. Compile shader (shared by all members) ─────────────────
    unsigned int shader = createShaderProgram();

    // ── 3. Player / camera (Member 2) ────────────────────────────
    Player player;

    // ── 4. Enemy manager (Member 3) ───────────────────────────────
    EnemyManager enemies;

    // ── 5. Collision detector (Member 4) ──────────────────────────
    CollisionDetector collision;

    // ── 6. HUD (Member 5) ─────────────────────────────────────────
    HUD hud;

    // ── 7. Ground geometry ────────────────────────────────────────
    setupGround();

    // ── 8. Game state ─────────────────────────────────────────────
    GameState state = GameState::START;

    std::cout << "\n";
    std::cout << "=== DRONE SHOOTER (SIMPLIFIED) ===\n";
    std::cout << "  WASD       - Move\n";
    std::cout << "  Mouse      - Look around\n";
    std::cout << "  SPACE      - Shoot\n";
    std::cout << "  ENTER      - Start / Restart\n";
    std::cout << "  ESC        - Quit\n";
    std::cout << "==================================\n\n";

    // ── 9. Game Loop ──────────────────────────────────────────────
    while (!win.shouldClose())
    {
        float dt   = win.getDeltaTime();
        float time = static_cast<float>(glfwGetTime());

        // ── Clear the screen (sky blue background) ────────────────
        glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ──────────────────────────────────────────────────────────
        //  STATE: START — show title screen, wait for ENTER
        // ──────────────────────────────────────────────────────────
        if (state == GameState::START)
        {
            // Draw a minimal 3D view behind the overlay
            auto view = player.getViewMatrix();
            auto proj = player.getProjMatrix();
            drawGround(shader, view, proj);

            // Member 5: draw start screen overlay
            hud.drawStartScreen(shader, player.getOrthoMatrix(), time);

            if (glfwGetKey(win.handle, GLFW_KEY_ENTER) == GLFW_PRESS)
                state = GameState::PLAYING;
        }

        // ──────────────────────────────────────────────────────────
        //  STATE: PLAYING — full game
        // ──────────────────────────────────────────────────────────
        else if (state == GameState::PLAYING)
        {
            // Member 2: handle keyboard + mouse
            player.processInput(win.handle, dt);
            player.processMouse(win.handle);

            // Member 3: move and update enemies
            enemies.update(player.position, dt);

            // Member 4: check bullet and laser collisions
            collision.detect(player, enemies);

            // ── 3D scene rendering ────────────────────────────────
            auto view = player.getViewMatrix();
            auto proj = player.getProjMatrix();

            drawGround (shader, view, proj);       // static floor
            enemies.draw(shader, view, proj);       // Member 3 draws drones

            // ── 2D HUD rendering (depth test off) ─────────────────
            auto ortho = player.getOrthoMatrix();
            player.drawCrosshair(shader);           // Member 2: crosshair
            player.drawGunBar   (shader);           // Member 2: gun rect
            hud.drawHealthBar   (shader, ortho, player.health);   // Member 5
            hud.drawKillMarkers (shader, ortho, player.kills);    // Member 5

            // ── Transition to DEAD if health runs out ─────────────
            if (player.health <= 0.0f)
            {
                state = GameState::DEAD;
                enemies.reset();
            }
        }

        // ──────────────────────────────────────────────────────────
        //  STATE: DEAD — game over screen
        // ──────────────────────────────────────────────────────────
        else if (state == GameState::DEAD)
        {
            // Draw frozen 3D view in background
            auto view = player.getViewMatrix();
            auto proj = player.getProjMatrix();
            drawGround(shader, view, proj);

            // Member 5: draw game over overlay
            hud.drawGameOverScreen(shader, player.getOrthoMatrix(),
                                   player.kills, time);

            // ENTER = restart
            if (glfwGetKey(win.handle, GLFW_KEY_ENTER) == GLFW_PRESS)
            {
                player.reset();
                enemies.reset();
                state = GameState::PLAYING;
            }
        }

        // End of frame: display and poll events (Member 1)
        win.swapAndPoll();
    }

    // Cleanup
    glDeleteVertexArrays(1, &g_groundVAO);
    glDeleteBuffers(1, &g_groundVBO);
    glDeleteProgram(shader);

    std::cout << "Game closed. Final kills: " << player.kills << "\n";
    return 0;
}
