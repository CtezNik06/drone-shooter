#pragma once

// ================================================================
// MEMBER 1: glfw_window.h  —  Window & Game Loop
// ================================================================
// YOUR JOB:
//   • Create the OS window and load OpenGL with GLAD
//   • Track elapsed time between frames (deltaTime)
//   • Each frame: poll input events and swap display buffers
//
// KEY CONCEPTS TO EXPLAIN (when presenting):
//   1. OpenGL Context  — the "workspace" OpenGL draws into.
//                        GLFW creates a window and attaches a context.
//   2. GLAD            — loads OpenGL function pointers at runtime
//                        (the actual GPU functions aren't linked at
//                        compile time; GLAD finds them for us).
//   3. Double Buffering — we draw into a hidden back-buffer, then
//                        swap it to the screen.  This prevents tearing.
//   4. Delta Time      — time since last frame.  Multiplying speed
//                        by delta time makes movement frame-rate
//                        independent (same speed at 30 FPS or 144 FPS).
// ================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class GLFWWindow {
public:
    GLFWwindow* handle;   // Raw GLFW pointer — other members use this
                          // to poll key presses (glfwGetKey).

    // Creates window + loads OpenGL.  width, height = pixel size.
    GLFWWindow(int width, int height, const char* title);

    // Cleans up GLFW when the program exits.
    ~GLFWWindow();

    // Returns true when the player presses ESC or clicks the X button.
    bool shouldClose() const;

    // END of every frame: show the back-buffer on screen and process
    // all pending OS events (keyboard, mouse, resize, close, …).
    void swapAndPoll();

    // Returns the number of seconds that elapsed since the last call.
    // Store this in `dt` and multiply all speeds by it.
    float getDeltaTime();

private:
    float m_lastTime;  // timestamp of the previous frame
};
