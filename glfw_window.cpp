// ================================================================
// MEMBER 1: glfw_window.cpp  —  Window & Game Loop
// ================================================================

#include "glfw_window.h"
#include "game_state.h"
#include <iostream>

GLFWWindow::GLFWWindow(int width, int height, const char* title)
{
    // ── Step 1: Initialise GLFW ──────────────────────────────────
    // GLFW is a cross-platform library that creates windows and
    // handles input.  glfwInit() must be called before anything else.
    if (!glfwInit()) {
        std::cerr << "ERROR: glfwInit() failed\n";
        exit(1);
    }

    // Tell GLFW we want an OpenGL 3.3 Core Profile context.
    // Core Profile = modern OpenGL (no deprecated legacy functions).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // Enable 4x MSAA

    // ── Step 2: Create the Window ────────────────────────────────
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    handle = glfwCreateWindow(mode->width, mode->height, title,
                              monitor,   // monitor (non-null = fullscreen)
                              nullptr);  // share context
    if (!handle) {
        std::cerr << "ERROR: glfwCreateWindow() failed\n";
        glfwTerminate();
        exit(1);
    }

    // Make this window's OpenGL context the "current" context.
    // All OpenGL calls go to this context until we switch.
    glfwMakeContextCurrent(handle);

    // ── Step 3: Load OpenGL Function Pointers with GLAD ──────────
    // OpenGL functions (like glClear, glDrawArrays, etc.) are not
    // available at link time — the GPU driver provides them.
    // GLAD queries the driver and stores the function pointers.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "ERROR: gladLoadGLLoader() failed\n";
        exit(1);
    }

    // Tell OpenGL the pixel dimensions of the rendering area.
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(handle, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    
    // Update global variables for orthographic projection and logic
    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;

    // Enable depth testing: objects closer to the camera occlude
    // objects further away.  Without this everything draws in order,
    // which looks wrong in 3D.
    glEnable(GL_DEPTH_TEST);
    
    // Enable Multisample Anti-Aliasing (MSAA)
    glEnable(GL_MULTISAMPLE);

    // Enable alpha blending (needed for transparent HUD overlays).
    // src color * src_alpha + dst color * (1 - src_alpha)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Lock and hide the cursor so mouse movement is captured freely.
    glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Record startup time for getDeltaTime()
    m_lastTime = static_cast<float>(glfwGetTime());
}

GLFWWindow::~GLFWWindow()
{
    // Release all GLFW resources when the program exits.
    glfwTerminate();
}

bool GLFWWindow::shouldClose() const
{
    // Returns true when the user presses ESC or clicks the X button.
    // Internally, we also call glfwSetWindowShouldClose() from the
    // Player class when ESC is pressed.
    return glfwWindowShouldClose(handle);
}

void GLFWWindow::swapAndPoll()
{
    // ── Double-Buffer Swap ────────────────────────────────────────
    // We rendered this frame into the BACK buffer.
    // glfwSwapBuffers flips back ↔ front so the frame appears on screen.
    glfwSwapBuffers(handle);

    // ── Event Polling ─────────────────────────────────────────────
    // Processes all queued OS events (key presses, mouse moves, etc.)
    // Without this call, the window would freeze and become unresponsive.
    glfwPollEvents();
}

float GLFWWindow::getDeltaTime()
{
    // glfwGetTime() returns the number of seconds since glfwInit().
    float now = static_cast<float>(glfwGetTime());
    float dt  = now - m_lastTime;   // time elapsed since last frame
    m_lastTime = now;               // remember current time for next frame
    return dt;
    // Typical values:
    //   60  FPS → dt ≈ 0.0167 s
    //   144 FPS → dt ≈ 0.0069 s
    // Using dt keeps gameplay the same speed regardless of frame rate.
}
