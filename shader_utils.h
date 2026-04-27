#pragma once

// ================================================================
// SHARED: shader_utils.h
// Compiles the one simple shader used by ALL team members.
// ================================================================
// HOW OPENGL SHADERS WORK (quick overview):
//
//   Vertex Shader   — runs once per vertex.  Applies the MVP
//                     matrix to transform 3D positions into screen
//                     space.  We write this in GLSL (a C-like lang).
//
//   Fragment Shader — runs once per pixel covered by a triangle.
//                     We just output a solid color stored in uColor.
//
//   Shader Program  — vertex + fragment shader linked together.
//                     We compile this ONCE in main() and share the
//                     returned unsigned int ID everywhere.
// ================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// ── Shader source code (embedded as string literals) ──────────

// Vertex shader:  transforms each vertex by the MVP matrix.
static const char* VERT_SRC = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;   // input: one vertex position
uniform mat4 MVP;                     // Model × View × Projection
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
}
)glsl";

// Fragment shader: every pixel gets the same color we set in C++.
static const char* FRAG_SRC = R"glsl(
#version 330 core
uniform vec4 uColor;       // RGBA color — set from C++ each draw call
out vec4 FragColor;
void main() {
    FragColor = uColor;
}
)glsl";

// ── Compile a single shader stage (vertex OR fragment) ─────────
inline unsigned int compileShader(GLenum type, const char* src) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(id, 512, nullptr, log);
        std::cerr << "[Shader compile error] " << log << "\n";
    }
    return id;
}

// ── Link vertex + fragment into a usable program ───────────────
// Call this ONCE in main(); pass the returned ID to every draw fn.
inline unsigned int createShaderProgram() {
    unsigned int vs   = compileShader(GL_VERTEX_SHADER,   VERT_SRC);
    unsigned int fs   = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "[Program link error] " << log << "\n";
    }

    // Shader objects are no longer needed once linked
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ── Uniform helpers ────────────────────────────────────────────
// The shader has two uniforms.  Call these after glUseProgram().

// Set the Model-View-Projection matrix.
inline void setMVP(unsigned int prog, const glm::mat4& mvp) {
    glUniformMatrix4fv(
        glGetUniformLocation(prog, "MVP"),
        1, GL_FALSE, glm::value_ptr(mvp));
}

// Set the draw color (r, g, b in range 0..1; a = alpha 0..1).
inline void setColor(unsigned int prog,
                     float r, float g, float b, float a = 1.0f) {
    glUniform4f(glGetUniformLocation(prog, "uColor"), r, g, b, a);
}
