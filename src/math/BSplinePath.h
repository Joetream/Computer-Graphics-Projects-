#pragma once
#include <vector>
#include <glm/glm.hpp>

class BSplinePath {
public:
    void setControlPoints(const std::vector<glm::vec3>& rawPoints, bool closedLoop = true);
    glm::vec3 eval(float t) const;
    glm::vec3 tangent(float t) const;
    void evalByDistance(float distance, glm::vec3& outPos, glm::vec3& outTangent) const;
    float totalLength() const { return m_totalLength; }
    int segmentCount() const { return static_cast<int>(m_ctrl.size()) - 3; }

private:
    std::vector<glm::vec3> m_ctrl;
    struct ArcSample { float t; float dist; };
    std::vector<ArcSample> m_lut;
    float m_totalLength = 0.0f;
};
