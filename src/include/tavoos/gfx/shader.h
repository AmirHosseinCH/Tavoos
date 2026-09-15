#pragma once

#include <tavoos/third_party/glad/glad.h>

#include <string>
#include <string_view>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Tavoos {

class Shader {
public:
    Shader() = default;
    ~Shader();
    Shader(Shader&&) = default;
    Shader& operator=(Shader&&) = default;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void compile(std::string_view, std::string_view);
    void use();
    void release();

    void setInt(const std::string&, const int);
    void setFloat(const std::string&, const float);
    void setVec4(const std::string&, const glm::vec4&);
    void setVec2(const std::string&, const glm::vec2&);
    void setVec4Array(const std::string&, const glm::vec4* values, int count);
    void setFloatArray(const std::string&, const float* values, int count);
    void setMat4(const std::string&, const glm::mat4&);

private:
    GLuint m_program{0};
    bool m_valid{false};
    std::unordered_map<std::string, GLint> m_uniformCache;

    GLuint compileShader(GLuint, std::string_view);

    GLint getUniformLocation(const std::string&);
};

}