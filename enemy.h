#pragma once

// ================================================================
// MEMBER 3 (UPGRADED): enemy.h
// NEW FEATURES:
//   • Two enemy types: SCOUT (fast, 1HP) and HEAVY (slow, 3HP)
//   • Enemy HP system — heavy drones need 3 hits to kill
//   • Wave system — enemies get harder each wave
//   • Rotor animation — spinning parts on each drone
//   • HP bar — each drone shows its remaining health above it
//   • Score returned on kill (Scout=100, Heavy=300)
// ================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// ── Enemy type enum ─────────────────────────────────────────────
enum class EnemyType {
    SCOUT,  // fast, 1 HP, cyan,  small  (0.8 × 0.3 × 0.8)
    HEAVY   // slow, 3 HP, red,   large  (1.4 × 0.6 × 1.4)
};

// ── A single drone enemy ────────────────────────────────────────
struct Enemy {
    glm::vec3  position;     // World position
    glm::vec3  bboxMin;      // AABB minimum (read by Member 4)
    glm::vec3  bboxMax;      // AABB maximum (read by Member 4)
    glm::vec3  laserDir;     // Normalized laser direction
    EnemyType  type;         // SCOUT or HEAVY
    int        hp;           // Current hit points
    int        maxHp;        // Maximum hit points (1 or 3)
    float      speed;        // Movement speed for this drone
    bool       active;       // false = dead / waiting to respawn
    bool       laserActive;  // true = laser visible this frame
    bool       canDamage;    // true = laser can hurt player
    float      attackTimer;  // time since last attack
    float      respawnTimer; // time dead (for respawn logic)
};

// ── Manages all enemies + the wave system ───────────────────────
class EnemyManager {
public:
    std::vector<Enemy> enemies;

    // ── Wave tracking (NEW) ──────────────────────────────────────
    int   waveNumber;       // current wave (starts at 1)
    bool  waveComplete;     // true when all enemies this wave are dead

    EnemyManager();
    ~EnemyManager();

    // Update enemy movement, attack timers, respawn logic.
    // time = total elapsed seconds (for rotor animation).
    void update(const glm::vec3& playerPos, float dt, float time);

    // Draw all active enemies (bodies, rotors, lasers, HP bars).
    void draw(unsigned int shader,
              const glm::mat4& view,
              const glm::mat4& proj,
              const glm::mat4& ortho);

    // Hit an enemy — reduces HP. Returns score awarded (0 if still alive).
    // Called by Member 4 (CollisionDetector).
    int  hitEnemy(Enemy& e);

    // Start the next wave — spawn fresh enemies matching waveNumber.
    void startNextWave();

    // Rebuild AABB (call each frame; Member 4 reads these).
    static void updateBBox(Enemy& e);

    // Reset for a new game.
    void reset();

    // Returns true when every enemy in the current wave isDead.
    bool allDead() const;

private:
    unsigned int m_cubeVAO, m_cubeVBO;
    unsigned int m_laserVAO, m_laserVBO;

    float m_currentTime; // stored for rotor spin animation

    void setupGeometry();
    void spawnEnemy(EnemyType type);
    glm::vec3 randomSpawn();

    // Draw one cube instance
    void drawCube(unsigned int shader,
                  const glm::mat4& view, const glm::mat4& proj,
                  const glm::vec3& pos,  const glm::vec3& scale,
                  float r, float g, float b, float a = 1.0f);

    // Draw the laser beam
    void drawLaser(unsigned int shader,
                   const glm::mat4& view, const glm::mat4& proj,
                   const glm::vec3& from, const glm::vec3& dir);

    // NEW: Draw 4 spinning rotor cubes around the drone body
    void drawRotors(unsigned int shader,
                    const glm::mat4& view, const glm::mat4& proj,
                    const glm::vec3& pos, float bodyRadius, float time);

    // NEW: Draw a health bar projected above the enemy in 2D
    void drawHPBar(unsigned int shader, const glm::mat4& ortho,
                   const glm::mat4& view, const glm::mat4& proj,
                   const Enemy& e);
};
