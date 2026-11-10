#include "math/BSplinePath.h"

#include <algorithm>

namespace {
constexpr int kArcSamples = 1000;
constexpr float kEpsilon = 1e-6f;

void basis(float s, float b[4], float db[4]) {
    const float s2 = s * s, s3 = s2 * s;
    b[0] = (1.0f - 3.0f * s + 3.0f * s2 - s3) / 6.0f;
    b[1] = (4.0f - 6.0f * s2 + 3.0f * s3) / 6.0f;
    b[2] = (1.0f + 3.0f * s + 3.0f * s2 - 3.0f * s3) / 6.0f;
    b[3] = s3 / 6.0f;
    db[0] = (-3.0f + 6.0f * s - 3.0f * s2) / 6.0f;
    db[1] = (-12.0f * s + 9.0f * s2) / 6.0f;
    db[2] = (3.0f + 6.0f * s - 9.0f * s2) / 6.0f;
    db[3] = s2 / 2.0f;
}
} // namespace

void BSplinePath::setControlPoints(const std::vector<glm::vec3>& rawPoints, bool closedLoop) {
    m_ctrl.clear(); m_lut.clear(); m_totalLength = 0.0f;
    if (rawPoints.empty()) return;
    if (closedLoop) {
        m_ctrl = rawPoints;
        for (int i = 0; i < 3; ++i) m_ctrl.push_back(rawPoints[static_cast<std::size_t>(i) % rawPoints.size()]);
    } else {
        m_ctrl.reserve(rawPoints.size() + 6);
        for (int i = 0; i < 3; ++i) m_ctrl.push_back(rawPoints.front());
        m_ctrl.insert(m_ctrl.end(), rawPoints.begin(), rawPoints.end());
        for (int i = 0; i < 3; ++i) m_ctrl.push_back(rawPoints.back());
    }
    m_lut.reserve(kArcSamples + 1);
    glm::vec3 previous = eval(0.0f);
    m_lut.push_back({0.0f, 0.0f});
    for (int i = 1; i <= kArcSamples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kArcSamples);
        const glm::vec3 current = eval(t);
        m_totalLength += glm::length(current - previous);
        m_lut.push_back({t, m_totalLength});
        previous = current;
    }
}

glm::vec3 BSplinePath::eval(float t) const {
    if (m_ctrl.size() < 4) return m_ctrl.empty() ? glm::vec3(0.0f) : m_ctrl.front();
    const int segments = static_cast<int>(m_ctrl.size()) - 3;
    t = std::clamp(t, 0.0f, 1.0f);
    const float scaled = t * static_cast<float>(segments);
    const int segment = std::min(static_cast<int>(scaled), segments - 1);
    const float s = scaled - static_cast<float>(segment);
    float b[4], db[4]; basis(s, b, db);
    glm::vec3 result(0.0f);
    for (int i = 0; i < 4; ++i) result += b[i] * m_ctrl[static_cast<std::size_t>(segment + i)];
    return result;
}

glm::vec3 BSplinePath::tangent(float t) const {
    if (m_ctrl.size() < 4) return glm::vec3(0.0f, 0.0f, 1.0f);
    const int segments = static_cast<int>(m_ctrl.size()) - 3;
    t = std::clamp(t, 0.0f, 1.0f);
    const float scaled = t * static_cast<float>(segments);
    const int segment = std::min(static_cast<int>(scaled), segments - 1);
    const float s = scaled - static_cast<float>(segment);
    float b[4], db[4]; basis(s, b, db);
    glm::vec3 derivative(0.0f);
    for (int i = 0; i < 4; ++i) derivative += db[i] * m_ctrl[static_cast<std::size_t>(segment + i)];
    const float length = glm::length(derivative);
    return length > kEpsilon ? derivative / length : glm::vec3(0.0f, 0.0f, 1.0f);
}

void BSplinePath::evalByDistance(float distance, glm::vec3& outPos, glm::vec3& outTangent) const {
    if (m_lut.empty() || m_totalLength <= kEpsilon) {
        outPos = eval(0.0f); outTangent = tangent(0.0f); return;
    }
    distance = std::clamp(distance, 0.0f, m_totalLength);
    const auto upper = std::lower_bound(m_lut.begin(), m_lut.end(), distance,
        [](const ArcSample& sample, float value) { return sample.dist < value; });
    if (upper == m_lut.begin()) {
        outPos = eval(upper->t); outTangent = tangent(upper->t); return;
    }
    if (upper == m_lut.end()) {
        outPos = eval(m_lut.back().t); outTangent = tangent(m_lut.back().t); return;
    }
    const ArcSample& right = *upper;
    const ArcSample& left = *(upper - 1);
    const float span = right.dist - left.dist;
    const float alpha = span > kEpsilon ? (distance - left.dist) / span : 0.0f;
    const float t = left.t + alpha * (right.t - left.t);
    outPos = eval(t); outTangent = tangent(t);
}
