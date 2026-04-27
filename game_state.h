#pragma once

// ================================================================
// SHARED: game_state.h  (UPGRADED)
// ================================================================

// ── Screen dimensions ───────────────────────────────────────────
const int   SCR_WIDTH    = 800;
const int   SCR_HEIGHT   = 600;

// ── World size ──────────────────────────────────────────────────
const float TERRAIN_SIZE = 25.0f;

// ── Player constants ────────────────────────────────────────────
const float PLAYER_HEIGHT    = 1.0f;
const float PLAYER_SPEED     = 5.0f;
const float PLAYER_SPRINT    = 9.0f;   // NEW: sprint speed (hold SHIFT)
const float PLAYER_MAX_HP    = 100.0f;
const float SHOT_COOLDOWN    = 0.35f;  // NEW: seconds between shots
const int   MAX_AMMO         = 10;     // NEW: ammo capacity
const float RELOAD_TIME      = 2.5f;   // NEW: reload duration (seconds)

// ── Enemy constants ─────────────────────────────────────────────
const float ENEMY_ATTACK_INT = 2.5f;   // seconds between laser attacks
const float ENEMY_DAMAGE     = 10.0f;  // damage per laser hit

// Scout drone: fast, 1 HP, small
const float SCOUT_SPEED  = 2.0f;
const int   SCOUT_HP     = 1;

// Heavy drone: slow, 3 HP, large, deal more damage
const float HEAVY_SPEED  = 1.0f;
const int   HEAVY_HP     = 3;
const float HEAVY_DAMAGE = 20.0f;     // Heavy hits harder

// ── Scoring ─────────────────────────────────────────────────────
const int SCORE_SCOUT    = 100;        // Points for killing a Scout
const int SCORE_HEAVY    = 300;        // Points for killing a Heavy

// ── Wave constants ──────────────────────────────────────────────
const float WAVE_PAUSE   = 3.0f;       // seconds between waves

// ── Game State ──────────────────────────────────────────────────
enum class GameState {
    START,    // Title screen — waiting for ENTER
    PLAYING,  // Active gameplay
    WAVE_END, // Brief pause between waves (NEW)
    DEAD      // Game-over screen
};
