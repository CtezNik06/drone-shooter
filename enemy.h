#pragma once

// ================================================================
// MEMBER 3: enemy.h  —  Enemy (Drone Shape, Movement, Laser)
// ================================================================
// YOUR JOB:
//   • Spawn drones at random positions around the player
//   • Move each drone toward the player every frame
//   • Every ENEMY_ATTACK_INT seconds, fire a laser at the player
//   • Draw each drone as a 3D box (cube) using OpenGL
//   • Draw the laser as a thin stretched box pointing at the player
//
// KEY CONCEPTS TO EXPLAIN (when presenting):
//   1. Model Matrix — positions/rotates/scales a shape in the world.
//      The CUBE geometry is centered at origin.  We translate it
//      to the enemy's world position, then scale it to the right size.
//      model = translate(identity, pos) × scale(identity, size)
//
//   2. MVP = Projection × View × Model
//      This is the FINAL matrix we pass to the vertex shader.
//      It converts each vertex: local space → world → camera → screen.
//
//   3. Simple Enemy AI — direction vector from enemy to player:
//        dir = normalize(playerPos - enemyPos)
//        enemyPos += dir * speed * dt
//      No pathfinding needed — straight-line movement is enough.
//
//   4. Laser Direction — normalized vector from enemy to player.
//      Member 4 (collision) uses this ray same vector.
// ================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// ── A single drone enemy ────────────────────────────────────────
struct Enemy {
    glm::vec3 position;    // World position of the drone
    glm::vec3 bboxMin;     // AABB minimum corner (used by collision)
    glm::vec3 bboxMax;     // AABB maximum corner
    glm::vec3 laserDir;    // Normalized direction of the laser beam
    bool      active;      // true = alive and flying
    bool      laserActive; // true = laser beam visible this frame
    bool      canDamage;   // true = laser can hurt player this frame
    float     attackTimer; // Time since last attack (seconds)
    float     respawnTimer;// Time since death (used to respawn)
};

// ── Manages all enemies (spawn, update, draw) ───────────────────
class EnemyManager {
public:
    std::vector<Enemy> enemies;  // The list of all drones

    EnemyManager();
    ~EnemyManager();

    // Spawn initial enemies, update movement + attack, then draw.
    // Call once per frame with the player's current world position.
    void update(const glm::vec3& playerPos, float dt);
    void draw  (unsigned int shader,
                const glm::mat4& view,
                const glm::mat4& proj);

    // Reset for a new game (called when player restarts)
    void reset();

    // Rebuild bounding box around enemy — called each frame.
    // MEMBER 4 reads bboxMin / bboxMax for collision testing.
    static void updateBBox(Enemy& e);

private:
    unsigned int m_cubeVAO, m_cubeVBO;    // Drone body geometry
    unsigned int m_laserVAO, m_laserVBO;  // Laser beam geometry

    void setupGeometry();    // Upload cube and laser to GPU (called once)
    void spawnEnemy();       // Add a new active enemy at a random position

    // Draw one instance of the cube at the given world transform.
    // r,g,b = colour of this cube
    void drawCube(unsigned int shader,
                  const glm::mat4& view,
                  const glm::mat4& proj,
                  const glm::vec3& pos,
                  const glm::vec3& scale,
                  float r, float g, float b);

    // Draw the laser beam from `from` in the direction `dir`.
    void drawLaser(unsigned int shader,
                   const glm::mat4& view,
                   const glm::mat4& proj,
                   const glm::vec3& from,
                   const glm::vec3& dir);

    // Generate a random spawn position (outside the inner safe zone).
    glm::vec3 randomSpawn();
};
