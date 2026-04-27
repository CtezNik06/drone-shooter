#pragma once

// ================================================================
// MEMBER 4: collision.h  —  Collision Detection
// ================================================================
// YOUR JOB:
//   • Define a Ray (origin + direction) — represents a bullet/laser
//   • Define an AABB (axis-aligned bounding box) — represents a hit zone
//   • Implement ray-AABB intersection: does this ray hit this box?
//   • Check if the player's bullet hits an enemy (ray from player)
//   • Check if an enemy laser hits the player (ray from enemy)
//
// KEY CONCEPTS TO EXPLAIN (when presenting):
//   1. AABB (Axis-Aligned Bounding Box)
//      The simplest 3D hitbox: a rectangular box whose sides are
//      always parallel to the world X, Y, Z axes.
//      Defined by two corners: minCorner and maxCorner.
//
//   2. Ray = origin + t * direction
//      A ray starts at `origin` and travels in `direction`.
//      The parameter t controls how far along the ray we are.
//      At t=0 → origin.  At t=1 → origin + direction.
//
//   3. Ray-AABB Intersection (Slab Method)
//      We treat the box as the intersection of 3 pairs of parallel planes
//      (one pair per axis: x-min/x-max, y-min/y-max, z-min/z-max).
//      For each axis we compute the t values where the ray enters/exits.
//      If all three intervals overlap, the ray hits the box.
//
//      The formula for axis i:
//        t_near_i = (min_i - origin_i) / direction_i
//        t_far_i  = (max_i - origin_i) / direction_i
//      tEnter = max(t_near_x, t_near_y, t_near_z)
//      tExit  = min(t_far_x,  t_far_y,  t_far_z)
//      Hit if: tEnter <= tExit  AND  tExit >= 0  AND  tEnter <= maxRange
// ================================================================

#include <glm/glm.hpp>
#include "player.h"
#include "enemy.h"

// ── A ray shot from origin in direction dir ────────────────────
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;  // Should be normalized (unit length)

    Ray(const glm::vec3& o, const glm::vec3& d)
        : origin(o), direction(glm::normalize(d)) {}
};

// ── Axis-Aligned Bounding Box ───────────────────────────────────
struct AABB {
    glm::vec3 minCorner;  // smallest x, y, z
    glm::vec3 maxCorner;  // largest  x, y, z

    AABB() {}
    AABB(const glm::vec3& mn, const glm::vec3& mx)
        : minCorner(mn), maxCorner(mx) {}
};

// ── Collision detection between player and all enemies ──────────
class CollisionDetector {
public:
    CollisionDetector() {}

    // Call every frame.  Modifies Player and EnemyManager as needed.
    void detect(Player& player, EnemyManager& enemies);

private:
    // Test: does the ray hit the given AABB within maxDist units?
    // Returns true on hit.
    bool rayHitsAABB(const Ray& ray, const AABB& box, float maxDist) const;

    // Player shooting → check bullet against all enemy AABBs
    void checkPlayerBullet(Player& player, EnemyManager& enemies);

    // Each enemy laser → check against player AABB
    void checkEnemyLasers(Player& player, EnemyManager& enemies);
};
