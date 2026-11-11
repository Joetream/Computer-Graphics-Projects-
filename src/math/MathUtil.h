#pragma once
#include <glm/glm.hpp>
#include <cstdint>

namespace MathUtil {
    glm::mat4 lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
    glm::mat4 perspective(float fovyRad, float aspect, float zNear, float zFar);
    glm::mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar);
    glm::mat4 translate(const glm::vec3& t);
    glm::mat4 scale(const glm::vec3& s);
    glm::mat4 rotateX(float rad);
    glm::mat4 rotateY(float rad);
    glm::mat4 rotateZ(float rad);
    glm::mat3 normalMatrix(const glm::mat4& model);
    glm::vec3 srgbToLinear(uint32_t hexRGB);
    bool verifyAgainstGLM();
}
