# Drone Shooter - Team Member Roles & Components

This document outlines the roles, responsibilities, and components assigned to each of the 5 team members. It is designed to help everyone understand how the different parts of the game engine tie together and what each person is expected to explain or manage.

---

## Member 1: Window & Application Lifecycle
**Primary File:** `glfw_window.cpp`

**Responsibilities:**
- **Context Management:** Initialising GLFW and setting up the OpenGL 3.3 Core Profile context.
- **Window Handling:** Creating the application window, handling fullscreen logic (using Framebuffer sizes and Retina display scaling), and capturing the mouse cursor.
- **Game Loop Foundations:** Polling for operating system events and swapping the double buffers.
- **Timing:** Calculating the delta time (`dt`) between frames to ensure smooth gameplay regardless of the framerate.
- **Graphics Pipeline Setup:** Loading OpenGL function pointers using GLAD and enabling essential states like Depth Testing, Alpha Blending, and Multisample Anti-Aliasing (MSAA).

---

## Member 2: Player & Camera Mechanics
**Primary File:** `player.cpp`

**Responsibilities:**
- **Camera & Movement:** Calculating the View and Projection matrices. Implementing WASD movement, mouse-look (pitch/yaw), sprinting mechanics, and procedural animations like camera head-bobbing.
- **Player State:** Managing the player's health, current score, and total kills.
- **Weapon Mechanics:** Handling the firing logic, including shot cooldowns, ammo capacity, and reloading countdowns.
- **First-Person Elements:** Rendering the player's crosshair, the gun model placeholder on screen, and dynamic muzzle flashes when firing.

---

## Member 3: Enemy AI & Rendering
**Primary File:** `enemy.cpp`

**Responsibilities:**
- **Wave System:** Spawning increasing numbers of enemies per wave and scaling their movement speed/difficulty.
- **Enemy Archetypes:** Designing the properties for different enemy types (e.g., fast/weak 'Scouts' and slow/strong 'Heavies').
- **Artificial Intelligence:** Updating enemy positions to follow the player and handling attack timers to shoot lasers.
- **3D Rendering:** Drawing the drones using primitive cubes, applying procedural animations (like rotating rotor blades using trigonometry and time), and rendering laser beams.
- **World-to-Screen Projection:** Projecting 3D world coordinates onto the 2D screen space to render dynamic floating HP bars above the enemies.

---

## Member 4: Collision Detection & Physics
**Primary File:** `collision.cpp`

**Responsibilities:**
- **Hitboxes:** Computing bounding boxes for the player and enemies.
- **Hit Detection:** Detecting when the player's shots intersect with an enemy's bounding box and reducing enemy HP/triggering death states.
- **Damage Taken:** Detecting when enemy attacks (lasers) hit the player and calculating the resulting damage based on enemy type.
- **Proximity Calculation:** Passing data to the HUD about the distance to the closest enemy (used to trigger the danger ring pulse).

---

## Member 5: HUD & 2D Overlays
**Primary File:** `hud.cpp`

**Responsibilities:**
- **2D Rendering System:** Disabling depth-testing to draw 2D orthographic UI elements perfectly on top of the 3D world.
- **Game State Screens:** Designing the Start Screen (with keyboard controls layout) and Game Over Screen (with score and kill counters).
- **Player Interface:** Drawing the segmented health bar and the dynamic ammo bar that changes color while reloading.
- **Mini-Radar:** Creating a dynamic map that translates relative 3D world coordinates into 2D radar positions to show the player where enemies are.
- **Dynamic Indicators:** Creating the pulsing "Danger Ring" when enemies get too close and drawing the Wave Banner when completing a level.

---

## Shared / Global Components
**Files:** `main.cpp`, `game_state.h`, `shader_utils.h`

- **`main.cpp`**: The main entry point. Initializes all the member classes, sets up the terrain/walls, and contains the core State Machine (START, PLAYING, WAVE_END, DEAD).
- **`game_state.h`**: The global configuration file containing constants for screen size, player speed, enemy damage, score values, and the `GameState` enum.
- **`shader_utils.h`**: Shared utility for compiling the vertex and fragment shaders, applying distance fog, and setting Model-View-Projection (MVP) matrices.
