cmake_minimum_required(VERSION 3.5)
project(DroneShooterSimple)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Source files (one per team member + shared) ──────────────────
set(SOURCES
    main.cpp            # Game loop (assembles all members' work)
    glfw_window.cpp     # Member 1 – Window & Game Loop
    player.cpp          # Member 2 – Player / Camera
    enemy.cpp           # Member 3 – Enemy AI & Rendering
    collision.cpp       # Member 4 – Collision Detection
    hud.cpp             # Member 5 – HUD & Game State
    glad.c       # OpenGL loader (reused from parent project)
)

# ── Reuse the include headers already in the parent project ──────
#  glad/, glm/, GLFW/, KHR/ are all there
include_directories(include)

# ── Find GLFW (already installed as part of the parent project) ──
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

include_directories(${GLFW_INCLUDE_DIRS})
link_directories   (${GLFW_LIBRARY_DIRS})

# ── Build the executable ─────────────────────────────────────────
add_executable(DroneShooterSimple ${SOURCES})

if (APPLE)
    # macOS (Apple) specific linking
    target_link_libraries(DroneShooterSimple
        ${GLFW_LIBRARIES}
        dl
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    # Linux / WSL specific linking
    target_link_libraries(DroneShooterSimple
        ${GLFW_LIBRARIES}
        GL      # OpenGL itself
        X11     # X11 window system (Linux/WSL)
        pthread # POSIX threads (needed by GLFW)
        dl      # dynamic library loader (needed by GLAD)
    )
endif()

# ── Output the binary to the simple/ directory ───────────────────
set_target_properties(DroneShooterSimple PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set(CMAKE_BUILD_TYPE Debug)
