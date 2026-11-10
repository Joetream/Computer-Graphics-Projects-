#include "core/Input.h"
#include <GLFW/glfw3.h>
#include <cstring>

void Input::init(GLFWwindow* window) {
    m_window = window;
    std::memset(m_currKeys, 0, sizeof(m_currKeys));
    std::memset(m_prevKeys, 0, sizeof(m_prevKeys));
    setCursorCaptured(true);
}

void Input::setCursorCaptured(bool captured) {
    m_captured = captured;
    m_firstMouse = true;
    if (m_window) {
        glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}

void Input::update() {
    if (!m_window) return;
    std::memcpy(m_prevKeys, m_currKeys, sizeof(m_currKeys));
    for (int k = 32; k < MAX_KEYS; ++k) {
        m_currKeys[k] = (glfwGetKey(m_window, k) == GLFW_PRESS);
    }

    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(m_window, &mx, &my);
    if (m_firstMouse) {
        m_lastX = mx;
        m_lastY = my;
        m_firstMouse = false;
    }
    if (m_captured) {
        m_dx = static_cast<float>(mx - m_lastX);
        m_dy = static_cast<float>(m_lastY - my); // reversed: y ranges bottom-to-top
    } else {
        m_dx = 0.0f;
        m_dy = 0.0f;
    }
    m_lastX = mx;
    m_lastY = my;
}

bool Input::pressed(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return m_currKeys[key] && !m_prevKeys[key];
}

bool Input::held(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return m_currKeys[key];
}
