#pragma once

// ================================================================
// SHARED: game_state.h
// This file is used by ALL 5 team members.
// It defines constants and the GameState enum.
// ================================================================

// ── Screen dimensions ──────────────────────────────────────────
const int   SCR_WIDTH    = 800;     // Window width  in pixels
const int   SCR_HEIGHT   = 600;     // Window height in pixels

// ── World size ─────────────────────────────────────────────────
// The level is a square of side 2*TERRAIN_SIZE centered at origin.
const float TERRAIN_SIZE = 25.0f;

// ── Player constants ───────────────────────────────────────────
const float PLAYER_HEIGHT = 1.0f;   // Camera is 1 unit above ground
const float PLAYER_SPEED  = 5.0f;   // Movement speed (units / second)
const float PLAYER_MAX_HP = 100.0f; // Starting health

// ── Enemy constants ────────────────────────────────────────────
const int   MAX_ENEMIES      = 3;    // How many drones can be alive at once
const float ENEMY_SPEED      = 1.5f; // Drone move speed (units / second)
const float ENEMY_DAMAGE     = 10.0f;// Damage per laser hit
const float ENEMY_ATTACK_INT = 3.0f; // Seconds between laser attacks

// ── The three possible stages of the game ─────────────────────
enum class GameState {
    START,    // Title / instructions screen — waiting for ENTER
    PLAYING,  // Active gameplay — player is alive
    DEAD      // Game-over screen — player ran out of health
};
