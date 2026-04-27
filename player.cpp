// ================================================================
// MEMBER 2: player.cpp  —  Player (Camera, Movement, Crosshair)
// ================================================================

#include "player.h"
#include "game_state.h"
#include "shader_utils.h"

#include <glm/gtc/matrix_transform.hpp>  // glm::lookAt, perspective, ortho
#include <algorithm>                      // std::clamp
#include <cmath>

// ── Constructor ──────────────────────────────────────────────────
Player::Player()
{
    reset();
    // Build VAO/VBO once — geometry doesn't change so we do this here.
    setupCrosshairGeometry();
    setupGunBarGeometry();
}

// ── Destructor ───────────────────────────────────────────────────
Player::~Player()
{
    glDeleteVertexArrays(1, &m_crosshairVAO);
    glDeleteBuffers(1, &m_crosshairVBO);
    glDeleteVertexArrays(1, &m_gunBarVAO);
    glDeleteBuffers(1, &m_gunBarVBO);
}

// ── Reset (called at game start and after death) ─────────────────
void Player::reset()
{
    // Start position: centre of world, at standing height
    position    = glm::vec3(0.0f, PLAYER_HEIGHT, 0.0f);

    // Default look direction: forward along -Z axis
    front  = glm::vec3(0.0f, 0.0f, -1.0f);
    right  = glm::vec3(1.0f, 0.0f,  0.0f);
    up     = glm::vec3(0.0f, 1.0f,  0.0f);

    m_yaw        = -90.0f;  // -90° makes the camera look along -Z initially
    m_pitch      =   0.0f;
    m_firstMouse =  true;
    m_lastX      = SCR_WIDTH  / 2.0f;
    m_lastY      = SCR_HEIGHT / 2.0f;

    health = PLAYER_MAX_HP;
    kills  = 0;
    shot   = false;
    range  = TERRAIN_SIZE * 4.0f;  // bullet travels far enough to cross the map

    updateBoundingBox();
}

// ── Recompute front / right / up from yaw and pitch ──────────────
// This is the EULER ANGLE → DIRECTION VECTOR conversion.
// It runs every time the player moves the mouse.
void Player::updateVectors()
{
    // Standard trigonometric formula for a free-look camera:
    //   front.x = cos(yaw) × cos(pitch)
    //   front.y = sin(pitch)
    //   front.z = sin(yaw) × cos(pitch)
    glm::vec3 f;
    f.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    f.y = sin(glm::radians(m_pitch));
    f.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front = glm::normalize(f);

    // right = cross(front, world_up).  Normalize to keep it unit-length.
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

    // up = cross(right, front) — keeps camera "level" as pitch changes.
    up = glm::normalize(glm::cross(right, front));
}

// ── Keyboard Input ────────────────────────────────────────────────
void Player::processInput(GLFWwindow* win, float dt)
{
    shot = false;  // reset every frame — only true the frame SPACE fires

    // Close window
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    // Movement speed = PLAYER_SPEED units/second  ×  dt seconds this frame
    float speed = PLAYER_SPEED * dt;

    // We only use front.x and front.z (ignoring Y) so the player stays
    // on the ground even when looking up or down.
    glm::vec3 horizFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
        position += horizFront * speed;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
        position -= horizFront * speed;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
        position -= right * speed;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
        position += right * speed;

    // Keep player standing on the ground and inside the terrain boundary
    position.y = PLAYER_HEIGHT;
    position.x = std::clamp(position.x, -TERRAIN_SIZE + 1.0f, TERRAIN_SIZE - 1.0f);
    position.z = std::clamp(position.z, -TERRAIN_SIZE + 1.0f, TERRAIN_SIZE - 1.0f);

    // Shoot — SPACE key
    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
        shot = true;

    // Rebuild bounding box around the new position every frame
    updateBoundingBox();
}

// ── Mouse Look ───────────────────────────────────────────────────
void Player::processMouse(GLFWwindow* win)
{
    double xd, yd;
    glfwGetCursorPos(win, &xd, &yd);
    float xpos = static_cast<float>(xd);
    float ypos = static_cast<float>(yd);

    // Skip the first event to avoid a large jump from the initial position
    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
        return;
    }

    // How many pixels did the mouse move since last frame?
    float dx = xpos - m_lastX;
    float dy = m_lastY - ypos;  // inverted: moving mouse UP should look UP
    m_lastX  = xpos;
    m_lastY  = ypos;

    // Sensitivity: how many degrees per pixel
    const float sensitivity = 0.1f;
    m_yaw   += dx * sensitivity;
    m_pitch += dy * sensitivity;

    // Clamp pitch so we can't flip the camera over
    if (m_pitch >  88.0f) m_pitch =  88.0f;
    if (m_pitch < -88.0f) m_pitch = -88.0f;

    updateVectors();
}

// ── Called by CollisionDetector when a laser hits ────────────────
void Player::takeDamage(float amount)
{
    health -= amount;
    if (health < 0.0f) health = 0.0f;
}

// ── Axis-Aligned Bounding Box around player ──────────────────────
void Player::updateBoundingBox()
{
    // A box that is 0.6 × 1.5 × 0.6 units centred on player position
    bboxMin = position + glm::vec3(-0.3f, -0.75f, -0.3f);
    bboxMax = position + glm::vec3( 0.3f,  0.75f,  0.3f);
}

// ── VIEW Matrix (glm::lookAt) ─────────────────────────────────────
// lookAt(eye, centre, up):
//   eye    = camera world position
//   centre = point the camera looks at (eye + front)
//   up     = which direction is "up" in the scene
glm::mat4 Player::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

// ── 3D PERSPECTIVE Projection Matrix ─────────────────────────────
// glm::perspective(fov, aspect, near, far):
//   fov    = 45° field of view (in radians after conversion)
//   aspect = width / height  (800/600 ≈ 1.333)
//   near   = 0.1 units — geometry closer is clipped
//   far    = 300 units — geometry further is clipped
glm::mat4 Player::getProjMatrix() const
{
    return glm::perspective(glm::radians(45.0f),
                            (float)SCR_WIDTH / (float)SCR_HEIGHT,
                            0.1f, 300.0f);
}

// ── 2D ORTHOGRAPHIC Projection Matrix ────────────────────────────
// Pixel coordinate (px, py) maps directly to screen position.
// Used for HUD elements (health bar, crosshair, kill markers).
glm::mat4 Player::getOrthoMatrix() const
{
    return glm::ortho(0.0f, (float)SCR_WIDTH,
                      0.0f, (float)SCR_HEIGHT);
}

// ── Crosshair: a + shape drawn with two GL_LINES ─────────────────
// The vertices are in PIXEL SPACE (0..800, 0..600).
// The orthographic projection maps them straight to the screen.
void Player::setupCrosshairGeometry()
{
    float cx  = SCR_WIDTH  / 2.0f;  // centre x
    float cy  = SCR_HEIGHT / 2.0f;  // centre y
    float arm = 14.0f;              // half-length of each arm

    // Four vertices — two lines:
    //   Line 1 (horizontal):  left  → right
    //   Line 2 (vertical):    bottom → top
    float verts[] = {
        cx - arm, cy,       0.0f,
        cx + arm, cy,       0.0f,
        cx,       cy - arm, 0.0f,
        cx,       cy + arm, 0.0f,
    };

    glGenVertexArrays(1, &m_crosshairVAO);
    glGenBuffers     (1, &m_crosshairVBO);

    glBindVertexArray(m_crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // Attribute 0 = aPos in the vertex shader (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Player::drawCrosshair(unsigned int shader)
{
    // Switch to 2D mode using orthographic projection
    glm::mat4 mvp = getOrthoMatrix(); // model = identity (no extra transform)

    glUseProgram(shader);
    setMVP  (shader, mvp);
    setColor(shader, 1.0f, 1.0f, 1.0f);  // white crosshair

    glBindVertexArray(m_crosshairVAO);
    glDrawArrays(GL_LINES, 0, 4);         // 4 vertices = 2 lines
    glBindVertexArray(0);
}

// ── Gun Bar: a dark rectangle at the bottom of the screen ────────
void Player::setupGunBarGeometry()
{
    // A flat rectangle in the bottom-centre area to suggest a gun body
    float x1 = SCR_WIDTH  * 0.40f;
    float x2 = SCR_WIDTH  * 0.60f;
    float y1 = 0.0f;
    float y2 = SCR_HEIGHT * 0.10f;

    // Two triangles forming one rectangle (quad)
    float verts[] = {
        x1, y1, 0.0f,   x2, y1, 0.0f,   x2, y2, 0.0f,
        x2, y2, 0.0f,   x1, y2, 0.0f,   x1, y1, 0.0f,
    };

    glGenVertexArrays(1, &m_gunBarVAO);
    glGenBuffers     (1, &m_gunBarVBO);
    glBindVertexArray(m_gunBarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gunBarVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Player::drawGunBar(unsigned int shader)
{
    glm::mat4 mvp = getOrthoMatrix();
    glUseProgram(shader);
    setMVP  (shader, mvp);
    setColor(shader, 0.25f, 0.25f, 0.25f);  // dark grey gun

    glBindVertexArray(m_gunBarVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
