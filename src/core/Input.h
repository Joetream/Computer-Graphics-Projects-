#pragma once
struct GLFWwindow;

class Input {
public:
    void init(GLFWwindow* window);
    void update();

    bool pressed(int key) const; // Edge trigger: true only on the frame key transitions up -> down
    bool held(int key) const;    // Level trigger: true while key is held down

    float mouseDeltaX() const { return m_dx; }
    float mouseDeltaY() const { return m_dy; }
    bool cursorCaptured() const { return m_captured; }
    void setCursorCaptured(bool captured);

private:
    static constexpr int MAX_KEYS = 512;
    GLFWwindow* m_window = nullptr;
    bool m_currKeys[MAX_KEYS] = {};
    bool m_prevKeys[MAX_KEYS] = {};
    bool m_firstMouse = true;
    bool m_captured = true;
    double m_lastX = 0.0;
    double m_lastY = 0.0;
    float m_dx = 0.0f;
    float m_dy = 0.0f;
};
