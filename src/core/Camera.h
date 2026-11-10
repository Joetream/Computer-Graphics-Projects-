#pragma once
#include <glm/glm.hpp>

class Input;
class BSplinePath;

class Camera {
public:
    Camera();

    void updateFreeRoam(const Input& input, float dt);
    void updateTour(const BSplinePath& path, float dt);

    void setTouring(bool touring) { m_touring = touring; }
    bool isTouring() const { return m_touring; }

    void setOrtho(bool ortho) { m_ortho = ortho; }
    bool isOrtho() const { return m_ortho; }

    void setPose(const glm::vec3& pos, float yawDeg, float pitchDeg);

    glm::vec3 position() const { return m_ortho ? glm::vec3(0.0f, 120.0f, 15.0f) : m_pos; }
    glm::vec3 front() const { return m_front; }
    glm::mat4 viewMatrix() const;
    glm::mat4 projMatrix(float aspect) const;

private:
    glm::vec3 m_pos{0.0f, 4.2f, 38.0f};
    glm::vec3 m_front{0.0f, -0.08f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};

    float m_yaw = -90.0f;
    float m_pitch = -5.0f;
    float m_moveSpeed = 16.0f;
    float m_mouseSens = 0.12f;
    float m_fovDeg = 52.0f;

    bool m_touring = false;
    bool m_ortho = false;
    float m_tourDist = 0.0f;
    float m_tourSpeed = 14.0f;

    void updateVectors();
};
