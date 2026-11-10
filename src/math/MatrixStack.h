#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class MatrixStack {
public:
    MatrixStack() { clear(); }
    void clear() { m_stack.clear(); m_stack.push_back(glm::mat4(1.0f)); }
    void push() { m_stack.push_back(m_stack.back()); }
    void push(const glm::mat4& m) { m_stack.push_back(m_stack.back() * m); }
    void pop() { if (m_stack.size() > 1) m_stack.pop_back(); }
    const glm::mat4& top() const { return m_stack.back(); }
    void mul(const glm::mat4& m) { m_stack.back() = m_stack.back() * m; }
    void translate(float x, float y, float z) { mul(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z))); }
    void rotate(float deg, const glm::vec3& axis) { mul(glm::rotate(glm::mat4(1.0f), glm::radians(deg), axis)); }
    void scale(float x, float y, float z) { mul(glm::scale(glm::mat4(1.0f), glm::vec3(x, y, z))); }
private:
    std::vector<glm::mat4> m_stack;
};
