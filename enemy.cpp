// ================================================================
// MEMBER 3: enemy.cpp  —  Enemy (Drone Shape, Movement, Laser)
// ================================================================

#include "enemy.h"
#include "game_state.h"
#include "shader_utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>   // rand()
#include <ctime>     // time()

// ── Cube vertex data (36 vertices, 6 faces × 2 triangles × 3 verts) ──
// The cube is centred at the origin, side length = 1.
// We scale it with the model matrix to set the actual size.
static const float CUBE_VERTS[] = {
    // Front face  (z = +0.5)
    -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,
    // Back face   (z = -0.5)
    -0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
    // Left face   (x = -0.5)
    -0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,  -0.5f, -0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
    // Right face  (x = +0.5)
     0.5f,  0.5f,  0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,   0.5f,  0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
    // Top face    (y = +0.5)
    -0.5f,  0.5f, -0.5f,   0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f,
    // Bottom face (y = -0.5)
    -0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,   0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,  -0.5f, -0.5f, -0.5f,  -0.5f, -0.5f,  0.5f,
};

// ── Constructor ──────────────────────────────────────────────────
EnemyManager::EnemyManager()
{
    srand(static_cast<unsigned int>(time(nullptr)));  // seed random numbers
    setupGeometry();

    // Spawn initial enemies
    enemies.clear();
    for (int i = 0; i < MAX_ENEMIES; ++i)
        spawnEnemy();
}

EnemyManager::~EnemyManager()
{
    glDeleteVertexArrays(1, &m_cubeVAO);
    glDeleteBuffers     (1, &m_cubeVBO);
    glDeleteVertexArrays(1, &m_laserVAO);
    glDeleteBuffers     (1, &m_laserVBO);
}

// ── Upload cube geometry to GPU once ────────────────────────────
void EnemyManager::setupGeometry()
{
    // ── Cube VAO/VBO ──
    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers     (1, &m_cubeVBO);

    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);

    // Attribute 0 = aPos in VERT_SRC shader
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Laser: same cube geometry, just scaled to a thin rod ──
    glGenVertexArrays(1, &m_laserVAO);
    glGenBuffers     (1, &m_laserVBO);
    glBindVertexArray(m_laserVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_laserVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ── Spawn a new enemy at a random edge position ─────────────────
void EnemyManager::spawnEnemy()
{
    Enemy e;
    e.position    = randomSpawn();
    e.active      = true;
    e.laserActive = false;
    e.canDamage   = false;
    e.attackTimer = 0.0f;
    e.respawnTimer= 0.0f;
    updateBBox(e);
    enemies.push_back(e);
}

// ── Generate a random position on the edges of the terrain ──────
glm::vec3 EnemyManager::randomSpawn()
{
    // Drones spawn in the outer ring of the map, 8–20 units from centre
    float minD = 8.0f;
    float maxD = TERRAIN_SIZE - 2.0f;
    float dist = minD + static_cast<float>(rand()) /
                 static_cast<float>(RAND_MAX) * (maxD - minD);

    // Random angle around the circle
    float angle = static_cast<float>(rand()) /
                  static_cast<float>(RAND_MAX) * 6.28318f;  // 0..2π

    float x = dist * cos(angle);
    float z = dist * sin(angle);
    float y = 2.5f + static_cast<float>(rand()) /
                     static_cast<float>(RAND_MAX) * 2.0f;   // 2.5..4.5 high

    return glm::vec3(x, y, z);
}

// ── Rebuild AABB each frame ──────────────────────────────────────
void EnemyManager::updateBBox(Enemy& e)
{
    // Drone body is drawn scaled to (1.2 × 0.5 × 1.2)
    const glm::vec3 half(0.6f, 0.35f, 0.6f);
    e.bboxMin = e.position - half;
    e.bboxMax = e.position + half;
}

// ── Update (movement + attack timer + respawn) ───────────────────
void EnemyManager::update(const glm::vec3& playerPos, float dt)
{
    for (auto& e : enemies) {
        if (!e.active) {
            // Wait for respawn timer, then re-activate
            e.respawnTimer += dt;
            if (e.respawnTimer >= 3.0f) {
                e.position     = randomSpawn();
                e.active       = true;
                e.laserActive  = false;
                e.canDamage    = false;
                e.attackTimer  = 0.0f;
                e.respawnTimer = 0.0f;
                updateBBox(e);
            }
            continue;
        }

        // ── Move toward player ────────────────────────────────────
        // Compute direction vector from enemy to player (on X-Z plane)
        glm::vec3 dir = playerPos - e.position;
        dir.y = 0.0f;
        float dist = glm::length(dir);

        // Only move if not already on top of the player
        if (dist > 0.5f) {
            glm::vec3 normDir = dir / dist;  // normalize manually
            e.position.x += normDir.x * ENEMY_SPEED * dt;
            e.position.z += normDir.z * ENEMY_SPEED * dt;
        }

        // ── Attack countdown ──────────────────────────────────────
        e.attackTimer += dt;
        e.laserActive  = false;
        e.canDamage    = false;

        if (e.attackTimer >= ENEMY_ATTACK_INT) {
            // Compute laser direction toward player (with slight random offset
            // so the enemy doesn't have perfect aim)
            glm::vec3 toPlayer = playerPos - e.position;
            float d = glm::length(toPlayer);
            if (d > 0.001f) {
                e.laserDir = toPlayer / d;
                // Add ±0.08 random offset so not every shot hits
                float offset = -0.08f + static_cast<float>(rand()) /
                               static_cast<float>(RAND_MAX) * 0.16f;
                e.laserDir.x += offset;
                e.laserDir.y += offset * 0.5f;
                e.laserDir    = glm::normalize(e.laserDir);
            }
            e.laserActive = true;
            e.canDamage   = true;
            e.attackTimer = 0.0f;  // reset timer
        }

        updateBBox(e);
    }
}

// ── Draw all enemies ─────────────────────────────────────────────
void EnemyManager::draw(unsigned int shader,
                         const glm::mat4& view,
                         const glm::mat4& proj)
{
    for (auto& e : enemies) {
        if (!e.active) continue;

        // Draw body — flat cube (teal / dark cyan colour)
        drawCube(shader, view, proj,
                 e.position,
                 glm::vec3(1.2f, 0.5f, 1.2f),  // scale: wide and flat
                 0.1f, 0.55f, 0.55f);            // teal

        // Draw "eye" indicator — small orange cube slightly in front
        glm::vec3 eyePos = e.position + glm::vec3(0.0f, 0.0f, -0.5f);
        drawCube(shader, view, proj,
                 eyePos,
                 glm::vec3(0.25f, 0.25f, 0.25f),
                 1.0f, 0.5f, 0.0f);             // orange

        // Draw laser if firing
        if (e.laserActive) {
            drawLaser(shader, view, proj, e.position, e.laserDir);
        }
    }
}

// ── Draw one cube instance ────────────────────────────────────────
// This demonstrates the MODEL matrix in action:
//   model = translate(identity, pos) × scale(identity, scl)
//   MVP   = projection × view × model
void EnemyManager::drawCube(unsigned int shader,
                              const glm::mat4& view,
                              const glm::mat4& proj,
                              const glm::vec3& pos,
                              const glm::vec3& scl,
                              float r, float g, float b)
{
    // ── Build model matrix ────────────────────────────────────────
    glm::mat4 model = glm::mat4(1.0f);               // start: identity
    model = glm::translate(model, pos);               // move to world pos
    model = glm::scale    (model, scl);               // scale to drone size

    // MVP combines all three transforms into one matrix
    glm::mat4 mvp = proj * view * model;

    glUseProgram(shader);
    setMVP  (shader, mvp);
    setColor(shader, r, g, b);

    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);  // 36 vertices = 12 triangles = 6 faces
    glBindVertexArray(0);
}

// ── Draw a thin laser beam in the given direction ─────────────────
// We use the same cube geometry, scaled to (0.05 × 0.05 × 15) so it
// becomes a thin rod.  We then rotate it to point along `dir`.
void EnemyManager::drawLaser(unsigned int shader,
                               const glm::mat4& view,
                               const glm::mat4& proj,
                               const glm::vec3& from,
                               const glm::vec3& dir)
{
    const float laserLen = 15.0f;

    // Centre the laser halfway along its length
    glm::vec3 laserCentre = from + dir * (laserLen / 2.0f);

    // Build model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, laserCentre);

    // Rotate from default (0,0,1) to the laser direction
    glm::vec3 defaultFwd(0.0f, 0.0f, 1.0f);
    glm::vec3 target = glm::normalize(dir);
    float cosA = glm::dot(defaultFwd, target);
    glm::vec3 axis = glm::cross(defaultFwd, target);
    if (glm::length(axis) > 0.001f) {
        float angle = acos(glm::clamp(cosA, -1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::normalize(axis));
    }

    // Scale: very thin in x/y, long in z
    model = glm::scale(model, glm::vec3(0.06f, 0.06f, laserLen));

    glm::mat4 mvp = proj * view * model;

    glUseProgram(shader);
    setMVP  (shader, mvp);
    setColor(shader, 1.0f, 0.1f, 0.1f);  // bright red laser

    glBindVertexArray(m_laserVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// ── Reset for a new game ─────────────────────────────────────────
void EnemyManager::reset()
{
    enemies.clear();
    for (int i = 0; i < MAX_ENEMIES; ++i)
        spawnEnemy();
}
