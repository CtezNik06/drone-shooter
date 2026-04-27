// ================================================================
// MEMBER 2 (UPGRADED): player.cpp
// NEW FEATURES: ammo/reload, shot cooldown, sprint, muzzle flash,
//               camera head-bob, score tracking
// ================================================================

#include "player.h"
#include "game_state.h"
#include "shader_utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>   // std::clamp
#include <cmath>       // sin, cos
#include <cstdlib>     // system

// ── Constructor ─────────────────────────────────────────────────
Player::Player()
{
    reset();
    setupCrosshairGeometry();
    setupGunBarGeometry();
    setupFlashGeometry();
}

Player::~Player()
{
    glDeleteVertexArrays(1, &m_crosshairVAO); glDeleteBuffers(1, &m_crosshairVBO);
    glDeleteVertexArrays(1, &m_gunBarVAO);    glDeleteBuffers(1, &m_gunBarVBO);
    glDeleteVertexArrays(1, &m_flashVAO);     glDeleteBuffers(1, &m_flashVBO);
}

// ── Reset ────────────────────────────────────────────────────────
void Player::reset()
{
    position    = glm::vec3(0.0f, PLAYER_HEIGHT, 0.0f);
    front       = glm::vec3(0.0f, 0.0f, -1.0f);
    right       = glm::vec3(1.0f, 0.0f,  0.0f);
    up          = glm::vec3(0.0f, 1.0f,  0.0f);
    m_yaw       = -90.0f;
    m_pitch     = 0.0f;
    m_firstMouse = true;
    m_lastX      = SCR_WIDTH  / 2.0f;
    m_lastY      = SCR_HEIGHT / 2.0f;
    m_bobAngle   = 0.0f;

    health       = PLAYER_MAX_HP;
    kills        = 0;
    score        = 0;
    shot         = false;
    range        = TERRAIN_SIZE * 4.0f;

    // Ammo
    ammo         = MAX_AMMO;
    reloading    = false;
    reloadTimer  = 0.0f;

    // Shot cooldown
    shotCooldown = 0.0f;

    // Muzzle flash
    muzzleFlash  = false;
    flashTimer   = 0.0f;

    // Movement
    isSprinting  = false;
    isMoving     = false;

    updateBoundingBox();
}

// ── Update Timers (call every frame) ─────────────────────────────
// NEW: Handles ammo reload countdown and muzzle flash countdown.
void Player::updateTimers(float dt)
{
    // ── Shot cooldown countdown ───
    if (shotCooldown > 0.0f)
        shotCooldown -= dt;

    // ── Muzzle flash countdown ────
    if (muzzleFlash) {
        flashTimer -= dt;
        if (flashTimer <= 0.0f)
            muzzleFlash = false;
    }

    // ── Ammo reload countdown ─────
    // NEW: Auto-reload takes RELOAD_TIME seconds.
    if (reloading) {
        reloadTimer -= dt;
        if (reloadTimer <= 0.0f) {
            // Reload complete — refill ammo
            ammo      = MAX_AMMO;
            reloading  = false;
            reloadTimer = 0.0f;
        }
    }

    // Auto-start reload when ammo runs out
    if (ammo <= 0 && !reloading) {
        reloading   = true;
        reloadTimer = RELOAD_TIME;
    }
}

// ── Recompute front/right/up from yaw and pitch ──────────────────
void Player::updateVectors()
{
    glm::vec3 f;
    f.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    f.y = sin(glm::radians(m_pitch));
    f.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up    = glm::normalize(glm::cross(right, front));
}

// ── Keyboard Input ────────────────────────────────────────────────
void Player::processInput(GLFWwindow* win, float dt)
{
    shot      = false;
    isMoving  = false;

    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    // ── Sprint (SHIFT key) — NEW ─────────────────────────────────
    // Sprint makes the player move faster and adds a stronger camera bob.
    isSprinting = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    float speed = (isSprinting ? PLAYER_SPRINT : PLAYER_SPEED) * dt;

    glm::vec3 horizFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) { position += horizFront * speed; isMoving = true; }
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) { position -= horizFront * speed; isMoving = true; }
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) { position -= right * speed;      isMoving = true; }
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) { position += right * speed;      isMoving = true; }

    // Keep player grounded and inside terrain
    position.y = PLAYER_HEIGHT;
    position.x = std::clamp(position.x, -TERRAIN_SIZE + 1.0f, TERRAIN_SIZE - 1.0f);
    position.z = std::clamp(position.z, -TERRAIN_SIZE + 1.0f, TERRAIN_SIZE - 1.0f);

    // ── Camera head-bob while moving — NEW ───────────────────────
    // The player's Y position oscillates slightly while walking/sprinting,
    // creating the feel of footsteps.
    if (isMoving) {
        float bobSpeed = isSprinting ? 18.0f : 11.0f;   // sprint bobs faster
        float bobAmp   = isSprinting ?  0.06f :  0.03f; // sprint bobs higher
        m_bobAngle += bobSpeed * dt;
        position.y = PLAYER_HEIGHT + sin(m_bobAngle) * bobAmp;
    } else {
        // Gently return to neutral when standing still
        m_bobAngle = 0.0f;
    }

    // ── Shooting (SPACE) with cooldown and ammo — NEW ────────────
    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
        // Can only shoot if: cooldown elapsed, has ammo, not reloading
        if (shotCooldown <= 0.0f && ammo > 0 && !reloading) {
            shot          = true;
            ammo--;                        // consume one bullet
            shotCooldown  = SHOT_COOLDOWN; // start cooldown timer
            muzzleFlash   = true;          // trigger flash effect
            flashTimer    = 0.08f;         // flash lasts 0.08 seconds
            system("afplay /System/Library/Sounds/Ping.aiff &");
        }
    }

    // Manual reload with R key — NEW
    if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS && !reloading && ammo < MAX_AMMO) {
        reloading   = true;
        reloadTimer = RELOAD_TIME;
        ammo        = 0;   // discard current clip
    }

    updateBoundingBox();
}

// ── Mouse Look ───────────────────────────────────────────────────
void Player::processMouse(GLFWwindow* win)
{
    double xd, yd;
    glfwGetCursorPos(win, &xd, &yd);

    if (m_firstMouse) {
        m_lastX = static_cast<float>(xd);
        m_lastY = static_cast<float>(yd);
        m_firstMouse = false;
        return;
    }

    float dx = static_cast<float>(xd) - m_lastX;
    float dy = m_lastY - static_cast<float>(yd);
    m_lastX  = static_cast<float>(xd);
    m_lastY  = static_cast<float>(yd);

    const float sensitivity = 0.1f;
    m_yaw   += dx * sensitivity;
    m_pitch += dy * sensitivity;
    if (m_pitch >  88.0f) m_pitch =  88.0f;
    if (m_pitch < -88.0f) m_pitch = -88.0f;

    updateVectors();
}

void Player::takeDamage(float amount)
{
    health -= amount;
    if (health < 0.0f) health = 0.0f;
}

void Player::updateBoundingBox()
{
    bboxMin = position + glm::vec3(-0.3f, -0.75f, -0.3f);
    bboxMax = position + glm::vec3( 0.3f,  0.75f,  0.3f);
}

// ── Matrix Getters ────────────────────────────────────────────────
glm::mat4 Player::getViewMatrix()  const { return glm::lookAt(position, position + front, up); }
glm::mat4 Player::getProjMatrix()  const { return glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 300.0f); }
glm::mat4 Player::getOrthoMatrix() const { return glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT); }

// ── Crosshair ────────────────────────────────────────────────────
void Player::setupCrosshairGeometry()
{
    float cx = SCR_WIDTH / 2.0f, cy = SCR_HEIGHT / 2.0f, arm = 14.0f;
    float verts[] = {
        cx-arm, cy,    0.0f,   cx+arm, cy,    0.0f,
        cx,  cy-arm,   0.0f,   cx,     cy+arm, 0.0f,
    };
    glGenVertexArrays(1, &m_crosshairVAO); glGenBuffers(1, &m_crosshairVBO);
    glBindVertexArray(m_crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Player::drawCrosshair(unsigned int shader)
{
    glm::mat4 mvp = getOrthoMatrix();
    glUseProgram(shader);
    setMVP(shader, mvp);

    // Crosshair turns red when reloading, yellow when almost no ammo, white normally
    if (reloading)         setColor(shader, 1.0f, 0.4f, 0.0f);  // orange = reloading
    else if (ammo <= 2)    setColor(shader, 1.0f, 1.0f, 0.0f);  // yellow = low ammo
    else                   setColor(shader, 1.0f, 1.0f, 1.0f);  // white  = normal

    glBindVertexArray(m_crosshairVAO);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
}

// ── Gun Bar ──────────────────────────────────────────────────────
void Player::setupGunBarGeometry()
{
    float x1=SCR_WIDTH*0.40f, x2=SCR_WIDTH*0.60f, y1=0.0f, y2=SCR_HEIGHT*0.10f;
    float verts[] = {
        x1,y1,0.0f, x2,y1,0.0f, x2,y2,0.0f,
        x2,y2,0.0f, x1,y2,0.0f, x1,y1,0.0f,
    };
    glGenVertexArrays(1, &m_gunBarVAO); glGenBuffers(1, &m_gunBarVBO);
    glBindVertexArray(m_gunBarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gunBarVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Player::drawGunBar(unsigned int shader)
{
    glUseProgram(shader);
    setMVP  (shader, getOrthoMatrix());
    setColor(shader, 0.25f, 0.25f, 0.25f);
    glBindVertexArray(m_gunBarVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── Muzzle Flash (NEW) ────────────────────────────────────────────
// When the player shoots, a large bright quad flashes at the crosshair.
// It fades out in about 0.08 seconds (controlled by flashTimer).
void Player::setupFlashGeometry()
{
    // A large quad centred on the screen
    float cx = SCR_WIDTH  / 2.0f;
    float cy = SCR_HEIGHT / 2.0f;
    float r  = 60.0f;  // radius of flash quad in pixels

    float verts[] = {
        cx-r, cy-r, 0.0f,   cx+r, cy-r, 0.0f,   cx+r, cy+r, 0.0f,
        cx+r, cy+r, 0.0f,   cx-r, cy+r, 0.0f,   cx-r, cy-r, 0.0f,
    };
    glGenVertexArrays(1, &m_flashVAO); glGenBuffers(1, &m_flashVBO);
    glBindVertexArray(m_flashVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_flashVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Player::drawMuzzleFlash(unsigned int shader)
{
    if (!muzzleFlash) return;

    // Alpha fades from 0.8 down to 0 over the flash duration (0.08 s)
    float alpha = flashTimer / 0.08f * 0.8f;
    alpha = std::clamp(alpha, 0.0f, 0.8f);

    glUseProgram(shader);
    setMVP  (shader, getOrthoMatrix());
    setColor(shader, 1.0f, 0.85f, 0.3f, alpha);  // bright yellow, semi-transparent

    glBindVertexArray(m_flashVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
