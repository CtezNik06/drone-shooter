// ================================================================
// MEMBER 3 (UPGRADED): enemy.cpp
// NEW: Two enemy types, HP system, waves, rotor animation, HP bars
// ================================================================

#include "enemy.h"
#include "game_state.h"
#include "shader_utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ── Unit cube vertices (36 verts, shared for body + rotors + laser) ──
static const float CUBE_VERTS[] = {
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
     0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f,-0.5f, 0.5f,
    -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,
     0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f,
};

// ── Constructor ──────────────────────────────────────────────────
EnemyManager::EnemyManager()
{
    srand(static_cast<unsigned int>(time(nullptr)));
    waveNumber    = 0;
    waveComplete  = false;
    m_currentTime = 0.0f;
    setupGeometry();
    startNextWave();   // Start wave 1
}

EnemyManager::~EnemyManager()
{
    glDeleteVertexArrays(1, &m_cubeVAO);  glDeleteBuffers(1, &m_cubeVBO);
    glDeleteVertexArrays(1, &m_laserVAO); glDeleteBuffers(1, &m_laserVBO);
}

// ── Upload cube geometry to GPU once ────────────────────────────
void EnemyManager::setupGeometry()
{
    auto makeVAO = [&](unsigned int& vao, unsigned int& vbo) {
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    };
    makeVAO(m_cubeVAO, m_cubeVBO);
    makeVAO(m_laserVAO, m_laserVBO);
}

// ── Wave system (NEW) ────────────────────────────────────────────
// Each wave spawns a different mix of Scout and Heavy drones.
// Wave 1: 3 Scouts
// Wave 2: 3 Scouts + 1 Heavy
// Wave 3: 4 Scouts + 2 Heavies
// Wave 4+: 3 Scouts + 3 Heavies (and they move faster each wave)
void EnemyManager::startNextWave()
{
    enemies.clear();
    waveNumber++;
    waveComplete = false;

    int scouts = 3;
    int heavies = 0;

    if (waveNumber == 2) { scouts = 3; heavies = 1; }
    else if (waveNumber == 3) { scouts = 4; heavies = 2; }
    else if (waveNumber >= 4) { scouts = 3; heavies = 3; }

    for (int i = 0; i < scouts;  ++i) spawnEnemy(EnemyType::SCOUT);
    for (int i = 0; i < heavies; ++i) spawnEnemy(EnemyType::HEAVY);
}

// ── Spawn a single enemy of the given type ───────────────────────
void EnemyManager::spawnEnemy(EnemyType type)
{
    Enemy e;
    e.type          = type;
    e.position      = randomSpawn();
    e.active        = true;
    e.laserActive   = false;
    e.canDamage     = false;
    e.attackTimer   = 0.0f;
    e.respawnTimer  = 0.0f;

    if (type == EnemyType::SCOUT) {
        e.hp    = SCOUT_HP;
        e.maxHp = SCOUT_HP;
        // Speed scales up with wave (faster in later waves)
        e.speed = SCOUT_SPEED + (waveNumber - 1) * 0.3f;
    } else {
        e.hp    = HEAVY_HP;
        e.maxHp = HEAVY_HP;
        e.speed = HEAVY_SPEED + (waveNumber - 1) * 0.15f;
    }

    updateBBox(e);
    enemies.push_back(e);
}

// ── Hit an enemy — reduce HP, return score if killed (NEW) ───────
// Returns: score gained (0 if still alive, SCORE_SCOUT/HEAVY if killed)
int EnemyManager::hitEnemy(Enemy& e)
{
    if (!e.active) return 0;
    e.hp--;
    if (e.hp <= 0) {
        e.active      = false;
        e.laserActive = false;
        return (e.type == EnemyType::SCOUT) ? SCORE_SCOUT : SCORE_HEAVY;
    }
    return 0;  // still alive
}

bool EnemyManager::allDead() const
{
    for (auto& e : enemies)
        if (e.active) return false;
    return true;
}

// ── Rebuild bounding box each frame ─────────────────────────────
void EnemyManager::updateBBox(Enemy& e)
{
    glm::vec3 half = (e.type == EnemyType::SCOUT)
                   ? glm::vec3(0.4f, 0.25f, 0.4f)
                   : glm::vec3(0.7f, 0.40f, 0.7f);
    e.bboxMin = e.position - half;
    e.bboxMax = e.position + half;
}

// ── Random spawn position (outer ring) ──────────────────────────
glm::vec3 EnemyManager::randomSpawn()
{
    float minD = 10.0f, maxD = TERRAIN_SIZE - 2.0f;
    float dist  = minD + static_cast<float>(rand()) / RAND_MAX * (maxD - minD);
    float angle = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
    float y     = 2.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
    return glm::vec3(dist*cos(angle), y, dist*sin(angle));
}

// ── Update: movement + attack + respawn ─────────────────────────
void EnemyManager::update(const glm::vec3& playerPos, float dt, float time)
{
    m_currentTime = time;

    for (auto& e : enemies) {
        if (!e.active) continue;

        // ── Move toward player ────────────────────────────────────
        glm::vec3 dir = playerPos - e.position;
        dir.y = 0.0f;
        float dist = glm::length(dir);
        if (dist > 0.5f) {
            glm::vec3 nd = dir / dist;
            e.position.x += nd.x * e.speed * dt;
            e.position.z += nd.z * e.speed * dt;
        }

        // ── Attack timer ──────────────────────────────────────────
        e.attackTimer += dt;
        e.laserActive  = false;
        e.canDamage    = false;

        // Heavy drones attack more frequently
        float interval = (e.type == EnemyType::HEAVY)
                       ? ENEMY_ATTACK_INT * 0.7f
                       : ENEMY_ATTACK_INT;

        if (e.attackTimer >= interval) {
            glm::vec3 toPlayer = playerPos - e.position;
            float d = glm::length(toPlayer);
            if (d > 0.001f) {
                e.laserDir = toPlayer / d;
                float off  = -0.07f + static_cast<float>(rand())/RAND_MAX * 0.14f;
                e.laserDir.x += off;
                e.laserDir.y += off * 0.4f;
                e.laserDir    = glm::normalize(e.laserDir);
            }
            e.laserActive = true;
            e.canDamage   = true;
            e.attackTimer = 0.0f;
        }

        updateBBox(e);
    }

    // Check if all enemies in this wave are defeated
    if (!waveComplete && allDead())
        waveComplete = true;
}

// ── Draw all enemies ─────────────────────────────────────────────
void EnemyManager::draw(unsigned int shader,
                         const glm::mat4& view,
                         const glm::mat4& proj,
                         const glm::mat4& ortho)
{
    for (auto& e : enemies) {
        if (!e.active) continue;

        if (e.type == EnemyType::SCOUT) {
            // Cyan flat body
            drawCube(shader, view, proj, e.position, {0.8f, 0.3f, 0.8f}, 0.05f, 0.75f, 0.75f);
            // White eye indicator (front)
            drawCube(shader, view, proj,
                     e.position + glm::vec3(0,0,-0.35f), {0.2f,0.2f,0.2f},
                     1.0f, 1.0f, 1.0f);
            // 4 spinning rotors
            drawRotors(shader, view, proj, e.position, 0.55f, m_currentTime);

        } else { // HEAVY
            // Dark red, large body
            drawCube(shader, view, proj, e.position, {1.4f, 0.6f, 1.4f}, 0.7f, 0.1f, 0.1f);
            // Orange eye
            drawCube(shader, view, proj,
                     e.position + glm::vec3(0,0,-0.65f), {0.3f,0.3f,0.3f},
                     1.0f, 0.4f, 0.0f);
            // Larger rotors
            drawRotors(shader, view, proj, e.position, 0.9f, m_currentTime * 0.7f);
        }

        // Red laser beam
        if (e.laserActive)
            drawLaser(shader, view, proj, e.position, e.laserDir);

        // HP bar above the drone (NEW)
        if (e.maxHp > 1)   // Only show for drones with more than 1 HP
            drawHPBar(shader, ortho, view, proj, e);
    }
}

// ── Draw one cube instance ────────────────────────────────────────
void EnemyManager::drawCube(unsigned int shader,
                              const glm::mat4& view, const glm::mat4& proj,
                              const glm::vec3& pos,  const glm::vec3& scl,
                              float r, float g, float b, float a)
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, scl);
    glUseProgram(shader);
    setMVP(shader, proj * view * model);
    setColor(shader, r, g, b, a);
    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// ── Rotor animation (NEW) ─────────────────────────────────────────
// Draws 4 small flat cubes at the corners of the drone body.
// Each rotor orbits around the drone center based on game time,
// creating the illusion of rotating blades.
//
// KEY CONCEPT: Using sin(time) and cos(time) on the X and Z offsets
// makes each rotor orbit in a circle around the drone's center.
void EnemyManager::drawRotors(unsigned int shader,
                               const glm::mat4& view, const glm::mat4& proj,
                               const glm::vec3& pos, float radius, float time)
{
    // Each rotor starts 90° apart and spins around the body
    const float rotorSpeed = 6.0f;   // radians per second
    float baseAngles[] = { 0.0f, 1.5708f, 3.1416f, 4.7124f }; // 0°, 90°, 180°, 270°

    for (int i = 0; i < 4; ++i) {
        float angle = baseAngles[i] + time * rotorSpeed;
        float rx = pos.x + radius * cos(angle);
        float rz = pos.z + radius * sin(angle);
        glm::vec3 rotorPos(rx, pos.y + 0.15f, rz);

        // Flat spinning disc: wide in X-Z, thin in Y
        drawCube(shader, view, proj, rotorPos,
                 glm::vec3(0.4f, 0.05f, 0.4f),
                 0.85f, 0.85f, 0.85f);  // light gray rotors
    }
}

// ── HP Bar above drone (NEW) ──────────────────────────────────────
// Projects the drone's 3D position to 2D screen space, then draws
// a health bar using the orthographic projection.
//
// KEY CONCEPT: 3D → 2D screen projection:
//   clipPos = Proj × View × vec4(worldPos, 1.0)
//   ndc      = clipPos.xyz / clipPos.w        ← perspective divide
//   screenX  = (ndc.x + 1) / 2 * SCR_WIDTH
//   screenY  = (ndc.y + 1) / 2 * SCR_HEIGHT
void EnemyManager::drawHPBar(unsigned int shader, const glm::mat4& ortho,
                               const glm::mat4& view, const glm::mat4& proj,
                               const Enemy& e)
{
    // Project point 1.2 units above the drone's centre
    glm::vec3 worldPos = e.position + glm::vec3(0.0f, 1.2f, 0.0f);
    glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.0f);

    // Reject if behind the camera
    if (clip.w <= 0.0f) return;

    // Perspective divide → Normalized Device Coordinates (-1 to +1)
    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    // Map NDC to pixel coordinates
    float sx = (ndc.x + 1.0f) * 0.5f * SCR_WIDTH;
    float sy = (ndc.y + 1.0f) * 0.5f * SCR_HEIGHT;

    // Skip if off screen
    if (sx < 0 || sx > SCR_WIDTH || sy < 0 || sy > SCR_HEIGHT) return;

    // Build a small MVP for each rectangle using ortho
    float barW = 50.0f, barH = 6.0f;
    float bx = sx - barW * 0.5f, by = sy;

    // Helper lambda to draw a rect
    auto drawRect = [&](float x, float y, float w, float h,
                        float r, float g, float b) {
        glm::mat4 model = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.f)),
            glm::vec3(w, h, 1.f));
        glUseProgram(shader);
        setMVP(shader, ortho * model);
        setColor(shader, r, g, b);
        glBindVertexArray(m_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    };

    glDisable(GL_DEPTH_TEST);
    drawRect(bx, by, barW, barH, 0.3f, 0.0f, 0.0f);  // dark bg
    float frac = static_cast<float>(e.hp) / e.maxHp;
    drawRect(bx, by, barW * frac, barH, 1.0f, 0.2f, 0.2f); // red HP
    glEnable(GL_DEPTH_TEST);
}

// ── Draw laser beam ───────────────────────────────────────────────
void EnemyManager::drawLaser(unsigned int shader,
                               const glm::mat4& view, const glm::mat4& proj,
                               const glm::vec3& from, const glm::vec3& dir)
{
    const float len = 18.0f;
    glm::vec3 centre = from + dir * (len * 0.5f);
    glm::mat4 model  = glm::translate(glm::mat4(1.0f), centre);
    glm::vec3 fwd(0,0,1);
    glm::vec3 tgt = glm::normalize(dir);
    float cosA = glm::dot(fwd, tgt);
    glm::vec3 axis = glm::cross(fwd, tgt);
    if (glm::length(axis) > 0.001f) {
        float angle = acos(std::clamp(cosA, -1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::normalize(axis));
    }
    model = glm::scale(model, {0.06f, 0.06f, len});

    glUseProgram(shader);
    setMVP(shader, proj * view * model);
    setColor(shader, 1.0f, 0.1f, 0.1f);
    glBindVertexArray(m_laserVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// ── Reset ────────────────────────────────────────────────────────
void EnemyManager::reset()
{
    enemies.clear();
    waveNumber   = 0;
    waveComplete = false;
    startNextWave();
}
