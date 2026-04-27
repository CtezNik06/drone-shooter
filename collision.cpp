// ================================================================
// MEMBER 4 (UPGRADED): collision.cpp
// Core algorithm unchanged — integrated with new HP system and score.
// ================================================================

#include "collision.h"
#include "game_state.h"
#include <algorithm>   // std::min, std::max, std::clamp
#include <cmath>       // std::abs

// ================================================================
// RAY-AABB INTERSECTION — THE SLAB METHOD
// (Algorithm is unchanged from the original version)
//
// For each of the 3 axes (X, Y, Z):
//   t_near = (slab_min - ray_origin) / ray_direction
//   t_far  = (slab_max - ray_origin) / ray_direction
// tEnter = max of all t_near values
// tExit  = min of all t_far values
// HIT if tEnter <= tExit and tExit >= 0 and tEnter <= maxDist
// ================================================================
bool CollisionDetector::rayHitsAABB(const Ray& ray,
                                     const AABB& box,
                                     float maxDist) const
{
    float tEnter = 0.0f;
    float tExit  = maxDist;

    for (int axis = 0; axis < 3; ++axis) {
        float o    = ray.origin   [axis];
        float d    = ray.direction[axis];
        float bMin = box.minCorner[axis];
        float bMax = box.maxCorner[axis];

        if (std::abs(d) < 1e-8f) {
            if (o < bMin || o > bMax) return false;
        } else {
            float t1 = (bMin - o) / d;
            float t2 = (bMax - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            tEnter = std::max(tEnter, t1);
            tExit  = std::min(tExit,  t2);
            if (tEnter > tExit) return false;
        }
    }
    return tExit >= 0.0f;
}

// ── Main detect — call every frame ──────────────────────────────
// Returns total score earned this frame.
int CollisionDetector::detect(Player& player, EnemyManager& enemies)
{
    int scoreGained = 0;

    if (player.shot)
        scoreGained += checkPlayerBullet(player, enemies);

    checkEnemyLasers(player, enemies);
    return scoreGained;
}

// ── Player Bullet ────────────────────────────────────────────────
// NEW: Calls enemies.hitEnemy() which decrements HP and returns
// score only when the enemy actually dies.  Heavy drones survive
// 3 hits before awarding score.
int CollisionDetector::checkPlayerBullet(Player& player,
                                          EnemyManager& enemies)
{
    int total = 0;
    Ray bullet(player.position, player.front);

    for (auto& e : enemies.enemies) {
        if (!e.active) continue;

        AABB box(e.bboxMin, e.bboxMax);
        if (rayHitsAABB(bullet, box, player.range)) {
            // hitEnemy() reduces HP by 1 and returns score if killed
            int gained = enemies.hitEnemy(e);
            if (gained > 0) {
                player.kills++;               // increment kill count
                player.score += gained;        // add score
            }
            total += gained;
        }
    }
    return total;
}

// ── Enemy Laser vs Player ─────────────────────────────────────────
// NEW: Heavy drones deal HEAVY_DAMAGE instead of regular ENEMY_DAMAGE.
void CollisionDetector::checkEnemyLasers(Player& player,
                                          EnemyManager& enemies)
{
    AABB playerBox(player.bboxMin, player.bboxMax);
    float laserRange = TERRAIN_SIZE * 4.0f;

    for (auto& e : enemies.enemies) {
        if (!e.active || !e.canDamage) continue;

        Ray laser(e.position, e.laserDir);
        if (rayHitsAABB(laser, playerBox, laserRange)) {
            // Heavy drones hit harder
            float dmg = (e.type == EnemyType::HEAVY) ? HEAVY_DAMAGE : ENEMY_DAMAGE;
            player.takeDamage(dmg);
            e.canDamage = false;  // one hit per attack cycle
        }
    }
}

// ── Helper: distance to nearest active enemy ─────────────────────
// Used by HUD to show a danger indicator when enemies are close.
float CollisionDetector::distToClosestEnemy(const Player& player,
                                             const EnemyManager& enemies) const
{
    float minDist = 9999.0f;
    for (auto& e : enemies.enemies) {
        if (!e.active) continue;
        float d = glm::length(e.position - player.position);
        if (d < minDist) minDist = d;
    }
    return minDist;
}
