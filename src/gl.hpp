/* danielsinkin97@gmail.com */
#pragma once

#include <cassert>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "types.hpp"
using TYPES::Rect;
using TYPES::COLOR::Color;

namespace GL {
using VAO = GLuint;
using VBO = GLuint;
using EBO = GLuint;
using ShaderID = GLuint;
using ProgramID = GLuint;
using UniformLocation = GLint;

class ShaderProgram {
public:
    ProgramID m_id = GL_ZERO;
    std::unordered_map<std::string, UniformLocation> m_uniforms;

    auto bind() const -> void {
        if (m_id == GL_ZERO) assert(false);
        glUseProgram(m_id);
    }
    static auto unbind() -> void { glUseProgram(GL_ZERO); }

    auto set_uniform(const std::string &name, float value) const -> void {
        glUniform1f(get_uniform(name), value);
    }

    auto set_uniform(const std::string &name, const glm::vec2 &v) const -> void {
        glUniform2f(get_uniform(name), v.x, v.y);
    }

    auto set_uniform(const std::string &name, const glm::vec3 &v) const -> void {
        glUniform3f(get_uniform(name), v.x, v.y, v.z);
    }

    auto load(const char *vertex_path, const char *fragment_path) -> void {
        const auto vert = compile_shader_from_file(vertex_path, GL_VERTEX_SHADER);
        const auto frag = compile_shader_from_file(fragment_path, GL_FRAGMENT_SHADER);

        m_id = glCreateProgram();
        glAttachShader(m_id, vert);
        glAttachShader(m_id, frag);
        glLinkProgram(m_id);

        GLint success;
        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[512];
            glGetProgramInfoLog(m_id, 512, nullptr, info_log);
            assert(false);
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

private:
    [[nodiscard]] auto get_uniform(const std::string &name) const -> UniformLocation {
        auto it = m_uniforms.find(name);
        if (it != m_uniforms.end()) return it->second;
        assert(false);
    }

    [[nodiscard]] auto compile_shader_from_file(const char *filepath, GLenum type) -> ShaderID {
        std::ifstream in(filepath);
        if (!in) {
            assert(false);
        }

        std::ostringstream ss;
        ss << in.rdbuf();
        const std::string source = ss.str();
        const char *src_ptr = source.c_str();

        ShaderID shader = glCreateShader(type);
        glShaderSource(shader, 1, &src_ptr, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char error_log[512];
            glGetShaderInfoLog(shader, 512, nullptr, error_log);
            assert(false);
        }

        return shader;
    }
};

struct GeometryBuffers {
    VAO vao = GL_ZERO;
    VBO vbo = GL_ZERO;
    EBO ebo = GL_ZERO;
};

template <size_t VertexCount, size_t IndexCount>
[[nodiscard]] inline auto create_geometry(
    const std::array<float, VertexCount> &vertices,
    const std::array<unsigned int, IndexCount> &indices) -> GeometryBuffers {
    GeometryBuffers gb;

    glGenVertexArrays(1, &gb.vao);
    glBindVertexArray(gb.vao);

    glGenBuffers(1, &gb.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gb.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &gb.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gb.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return gb;
}

inline auto set_box_uniforms(const ShaderProgram &sp, const Rect &box) -> void {
    sp.set_uniform("u_Pos", vec2{box.position.x, box.position.y});
    sp.set_uniform("u_Width", box.width);
    sp.set_uniform("u_Height", box.height);
}

inline auto set_color_uniforms(const ShaderProgram &sp, const Color &color) -> void {
    sp.set_uniform("u_Color", vec3{color.r, color.g, color.b});
}

inline auto draw_simple_vao(const GeometryBuffers &geom, GLsizei index_count) -> void {
    if (geom.vao == GL_ZERO || geom.ebo == GL_ZERO) {
        assert(false);
    }

    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
} // namespace GL