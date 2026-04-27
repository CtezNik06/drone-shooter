// ================================================================
// MEMBER 4: collision.cpp  —  Collision Detection (Slab Method)
// ================================================================

#include "collision.h"
#include "game_state.h"   // TERRAIN_SIZE, ENEMY_DAMAGE, etc.
#include <algorithm>      // std::min, std::max
#include <cmath>          // std::abs

// ================================================================
// RAY-AABB INTERSECTION  (the "Slab Method")
//
// A box can be thought of as the overlap of three infinite "slabs":
//
//        |   |          |   |          ---
//        |   |          |   |           |
//   -----| B |-----  X  |   | Y    --- Z ---
//        |   |          |   |           |
//        |   |          |   |          ---
//
// For each axis (X, Y, Z) we find the two values of t where the
// ray crosses that slab's walls.
//
// Entry into the box = max of all three entry t values.
// Exit  from the box = min of all three exit  t values.
//
// If entry <= exit, and exit >= 0, and entry <= maxRange → HIT!
// ================================================================

bool CollisionDetector::rayHitsAABB(const Ray& ray,
                                     const AABB& box,
                                     float maxDist) const
{
    float tEnter = 0.0f;        // largest  entry t across all axes
    float tExit  = maxDist;     // smallest exit  t across all axes

    for (int axis = 0; axis < 3; ++axis)
    {
        float o   = ray.origin   [axis];
        float d   = ray.direction[axis];
        float bMin = box.minCorner[axis];
        float bMax = box.maxCorner[axis];

        if (std::abs(d) < 1e-8f) {
            // Ray is parallel to this slab.
            // If origin is outside the slab → no intersection possible.
            if (o < bMin || o > bMax)
                return false;
            // Otherwise the ray travels inside this slab for its whole length;
            // skip this axis (contributing t = ±infinity).
        } else {
            // Compute intersections with the two slab planes for this axis.
            // t = (plane_position - origin) / direction
            float t1 = (bMin - o) / d;
            float t2 = (bMax - o) / d;

            // Make sure t1 is the near intersection, t2 the far one.
            if (t1 > t2) std::swap(t1, t2);

            // Shrink the [tEnter, tExit] interval to the intersection of
            // all three slabs.
            tEnter = std::max(tEnter, t1);
            tExit  = std::min(tExit,  t2);

            // Early exit: if the interval collapsed, no intersection.
            if (tEnter > tExit)
                return false;
        }
    }

    // tExit >= 0 ensures the box is in front of the ray, not behind it.
    return tExit >= 0.0f;
}

// ── Main detection function — called from main.cpp every frame ───
void CollisionDetector::detect(Player& player, EnemyManager& enemies)
{
    // Only test player bullet if they fired this frame
    if (player.shot)
        checkPlayerBullet(player, enemies);

    // Always check enemy lasers (they might be active)
    checkEnemyLasers(player, enemies);
}

// ── Player Bullet (ray from player along camera front vector) ─────
void CollisionDetector::checkPlayerBullet(Player& player,
                                           EnemyManager& enemies)
{
    // Bullet = ray starting at player's eye, going in the direction
    // the player is looking.
    Ray bullet(player.position, player.front);

    for (auto& e : enemies.enemies) {
        if (!e.active) continue;

        // Build AABB from the enemy's current bounding box
        AABB box(e.bboxMin, e.bboxMax);

        // Test the bullet ray against this enemy's box
        if (rayHitsAABB(bullet, box, player.range)) {
            // Hit! Kill the enemy and award a kill.
            e.active      = false;
            e.laserActive = false;
            e.canDamage   = false;
            player.kills++;
        }
    }
}

// ── Enemy Laser (ray from each enemy toward the player) ──────────
void CollisionDetector::checkEnemyLasers(Player& player,
                                          EnemyManager& enemies)
{
    // Player bounding box (updated each frame by Player::updateBoundingBox)
    AABB playerBox(player.bboxMin, player.bboxMax);

    for (auto& e : enemies.enemies) {
        // Only check if the enemy can actually deal damage this frame
        if (!e.active || !e.canDamage) continue;

        // Laser = ray from enemy center in precomputed laserDir
        Ray laser(e.position, e.laserDir);

        // Max laser range is large enough to cross the map
        float laserRange = TERRAIN_SIZE * 4.0f;

        if (rayHitsAABB(laser, playerBox, laserRange)) {
            // Laser hit the player
            player.takeDamage(ENEMY_DAMAGE);
            e.canDamage = false;  // prevent multiple hits per attack
        }
    }
}
