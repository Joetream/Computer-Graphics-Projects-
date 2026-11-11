#include "core/Camera.h"
#include "core/Input.h"
#include "math/MathUtil.h"
#include "math/BSplinePath.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

Camera::Camera() {
    updateVectors();
}

void Camera::setPose(const glm::vec3& pos, float yawDeg, float pitchDeg) {
    m_pos = pos;
    m_yaw = yawDeg;
    m_pitch = std::clamp(pitchDeg, -85.0f, 85.0f);
    updateVectors();
}

void Camera::updateVectors() {
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);
    glm::vec3 f;
    f.x = std::cos(yawRad) * std::cos(pitchRad);
    f.y = std::sin(pitchRad);
    f.z = std::sin(yawRad) * std::cos(pitchRad);
    m_front = glm::normalize(f);
    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::updateFreeRoam(const Input& input, float dt) {
    m_yaw   += input.mouseDeltaX() * m_mouseSens;
    m_pitch += input.mouseDeltaY() * m_mouseSens;
    m_pitch = std::clamp(m_pitch, -85.0f, 85.0f);
    updateVectors();

    float speed = m_moveSpeed * (input.held(GLFW_KEY_LEFT_SHIFT) ? 2.2f : 1.0f);
    float v = speed * dt;

    if (input.held(GLFW_KEY_W)) m_pos += m_front * v;
    if (input.held(GLFW_KEY_S)) m_pos -= m_front * v;
    if (input.held(GLFW_KEY_D)) m_pos += m_right * v;
    if (input.held(GLFW_KEY_A)) m_pos -= m_right * v;
    if (input.held(GLFW_KEY_E) || input.held(GLFW_KEY_SPACE)) m_pos.y += v;
    if (input.held(GLFW_KEY_Q)) m_pos.y -= v;

    if (m_pos.y < 1.2f) m_pos.y = 1.2f;
}

void Camera::updateTour(const BSplinePath& path, float dt) {
    if (path.totalLength() <= 1e-3f) return;
    m_tourDist += m_tourSpeed * dt;
    if (m_tourDist >= path.totalLength()) {
        m_tourDist -= path.totalLength();
    }
    glm::vec3 pos, tang;
    path.evalByDistance(m_tourDist, pos, tang);
    m_pos = pos;
    // Slight look-inward bias toward village center so houses stay framed nicely
    glm::vec3 toCenter = glm::normalize(glm::vec3(0.0f, 2.5f, 0.0f) - pos);
    m_front = glm::normalize( glm::mix(tang, toCenter, 0.28f) );
    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

glm::mat4 Camera::viewMatrix() const {
    if (m_ortho) {
        return MathUtil::lookAt(
            glm::vec3(0.0f, 135.0f, 18.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        );
    }
    return MathUtil::lookAt(m_pos, m_pos + m_front, m_up);
}

glm::mat4 Camera::projMatrix(float aspect) const {
    if (m_ortho) {
        float halfH = 95.0f;
        float halfW = halfH * aspect;
        return MathUtil::ortho(-halfW, halfW, -halfH, halfH, 1.0f, 350.0f);
    }
    return MathUtil::perspective(glm::radians(m_fovDeg), aspect, 0.25f, 420.0f);
}
