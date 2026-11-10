#include "math/MathUtil.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>

namespace MathUtil {
namespace {
float maxMatrixError(const glm::mat4& a, const glm::mat4& b) {
    float error = 0.0f;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            error = std::max(error, std::abs(a[col][row] - b[col][row]));
        }
    }
    return error;
}
} // namespace

glm::mat4 lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) {
    const glm::vec3 f = glm::normalize(center - eye);
    const glm::vec3 s = glm::normalize(glm::cross(f, up));
    const glm::vec3 u = glm::cross(s, f);
    glm::mat4 result(1.0f);
    result[0][0] = s.x; result[1][0] = s.y; result[2][0] = s.z;
    result[0][1] = u.x; result[1][1] = u.y; result[2][1] = u.z;
    result[0][2] = -f.x; result[1][2] = -f.y; result[2][2] = -f.z;
    result[3][0] = -glm::dot(s, eye);
    result[3][1] = -glm::dot(u, eye);
    result[3][2] = glm::dot(f, eye);
    return result;
}

glm::mat4 perspective(float fovyRad, float aspect, float zNear, float zFar) {
    const float tanHalfFovy = std::tan(fovyRad * 0.5f);
    glm::mat4 result(0.0f);
    result[0][0] = 1.0f / (aspect * tanHalfFovy);
    result[1][1] = 1.0f / tanHalfFovy;
    result[2][2] = -(zFar + zNear) / (zFar - zNear);
    result[2][3] = -1.0f;
    result[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return result;
}

glm::mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    glm::mat4 result(1.0f);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = -2.0f / (zFar - zNear);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -(zFar + zNear) / (zFar - zNear);
    return result;
}

glm::mat4 translate(const glm::vec3& t) {
    glm::mat4 result(1.0f);
    result[3] = glm::vec4(t, 1.0f);
    return result;
}

glm::mat4 scale(const glm::vec3& s) {
    glm::mat4 result(1.0f);
    result[0][0] = s.x; result[1][1] = s.y; result[2][2] = s.z;
    return result;
}

glm::mat4 rotateX(float rad) {
    const float c = std::cos(rad), s = std::sin(rad);
    glm::mat4 result(1.0f);
    result[1][1] = c; result[2][1] = s; result[1][2] = -s; result[2][2] = c;
    return result;
}

glm::mat4 rotateY(float rad) {
    const float c = std::cos(rad), s = std::sin(rad);
    glm::mat4 result(1.0f);
    result[0][0] = c; result[2][0] = -s; result[0][2] = s; result[2][2] = c;
    return result;
}

glm::mat4 rotateZ(float rad) {
    const float c = std::cos(rad), s = std::sin(rad);
    glm::mat4 result(1.0f);
    result[0][0] = c; result[1][0] = s; result[0][1] = -s; result[1][1] = c;
    return result;
}

glm::mat3 normalMatrix(const glm::mat4& model) {
    return glm::transpose(glm::inverse(glm::mat3(model)));
}

glm::vec3 srgbToLinear(uint32_t hexRGB) {
    const float r = static_cast<float>((hexRGB >> 16u) & 0xffu) / 255.0f;
    const float g = static_cast<float>((hexRGB >> 8u) & 0xffu) / 255.0f;
    const float b = static_cast<float>(hexRGB & 0xffu) / 255.0f;
    return glm::vec3(std::pow(r, 2.2f), std::pow(g, 2.2f), std::pow(b, 2.2f));
}

bool verifyAgainstGLM() {
    const glm::vec3 eye(3.0f, 4.0f, 7.0f);
    const glm::vec3 center(-1.0f, 0.5f, 0.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const float fovy = 1.1f, aspect = 16.0f / 9.0f, zNear = 0.25f, zFar = 420.0f;
    const float error = std::max({
        maxMatrixError(lookAt(eye, center, up), glm::lookAt(eye, center, up)),
        maxMatrixError(perspective(fovy, aspect, zNear, zFar), glm::perspective(fovy, aspect, zNear, zFar)),
        maxMatrixError(ortho(-5.0f, 7.0f, -3.0f, 4.0f, zNear, zFar), glm::ortho(-5.0f, 7.0f, -3.0f, 4.0f, zNear, zFar))
    });
    return error < 1e-5f;
}
} // namespace MathUtil
