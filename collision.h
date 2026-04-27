#pragma once

// ================================================================
// MEMBER 4 (UPGRADED): collision.h
// SAME core algorithm — gains these integration hooks:
//   • Calls EnemyManager::hitEnemy() instead of directly deactivating
//   • Returns score earned this frame to pass back to Player
//   • Handles Heavy drone's higher damage via EnemyType
//   • Added helper: distToClosestEnemy() — used for audio cue in HUD
// ================================================================

#include <glm/glm.hpp>
#include "player.h"
#include "enemy.h"

// ── A ray: starts at origin, travels in direction ───────────────
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    Ray(const glm::vec3& o, const glm::vec3& d)
        : origin(o), direction(glm::normalize(d)) {}
};

// ── Axis-Aligned Bounding Box ───────────────────────────────────
struct AABB {
    glm::vec3 minCorner;
    glm::vec3 maxCorner;
    AABB() {}
    AABB(const glm::vec3& mn, const glm::vec3& mx)
        : minCorner(mn), maxCorner(mx) {}
};

// ── Collision detector between player and all enemies ───────────
class CollisionDetector {
public:
    CollisionDetector() {}

    // Call every frame.
    // Returns the total score earned this frame from kills.
    int detect(Player& player, EnemyManager& enemies);

    // Returns distance to the nearest active enemy.
    // Used by HUD to trigger a danger indicator.
    float distToClosestEnemy(const Player& player,
                              const EnemyManager& enemies) const;

private:
    // Core math: does the ray intersect the AABB within maxDist?
    bool rayHitsAABB(const Ray& ray, const AABB& box,
                     float maxDist) const;

    // Player bullet vs all enemy AABBs — returns total score gained.
    int checkPlayerBullet(Player& player, EnemyManager& enemies);

    // Each enemy laser vs player AABB.
    void checkEnemyLasers(Player& player, EnemyManager& enemies);
};
