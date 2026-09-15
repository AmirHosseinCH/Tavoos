#include <tavoos/gfx/shader.h>

#include <array>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace Tavoos {

Shader::~Shader() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
}

void Shader::compile(std::string_view vertexShaderSource, std::string_view fragmentShaderSource) {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0)
            glDeleteShader(vertexShader);
        if (fragmentShader != 0)
            glDeleteShader(fragmentShader);
        return;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        std::array<char, 1024> log{};
        glGetProgramInfoLog(m_program, static_cast<GLsizei>(log.size()), nullptr, log.data());
        spdlog::error("shader link failed: {}", log.data());
        glDeleteProgram(m_program);
        m_program = 0;
        return;
    }

    m_valid = true;
}

void Shader::use() {
    if (!m_valid)
        throw std::runtime_error{"Cannot use shader program: the shader program is not valid"};

    glUseProgram(m_program);
}

void Shader::release() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void Shader::setInt(const std::string& name, const int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, const float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec4(const std::string& name, const glm::vec4& v) {
    glUniform4fv(getUniformLocation(name), 1, &v[0]);
}

void Shader::setVec2(const std::string& name, const glm::vec2& v) {
    glUniform2fv(getUniformLocation(name), 1, &v[0]);
}

void Shader::setVec4Array(const std::string& name, const glm::vec4* values, int count) {
    if (count <= 0) return;
    glUniform4fv(getUniformLocation(name), count, &values[0][0]);
}

void Shader::setFloatArray(const std::string& name, const float* values, int count) {
    if (count <= 0) return;
    glUniform1fv(getUniformLocation(name), count, values);
}

void Shader::setMat4(const std::string& name, const glm::mat4& m) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &m[0][0]);
}

GLuint Shader::compileShader(GLuint type, std::string_view src) {
    const GLuint shader = glCreateShader(type);
    const char* source = src.data();
    const GLint length = static_cast<GLuint>(src.size());

    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);

    int success{0};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        std::array<char, 1024> log{};
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        spdlog::error("shader compile failed: {}", log.data());
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLint Shader::getUniformLocation(const std::string& name) {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end())
        return it->second;

    GLint location = glGetUniformLocation(m_program, name.c_str());
    m_uniformCache[name] = location;
    return location;
}

}