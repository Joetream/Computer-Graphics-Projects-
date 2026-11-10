#include "App.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace {
App* g_app = nullptr;

void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    if (g_app) {
        g_app->onResize(width, height);
    }
}
} // namespace

int main(int argc, char** argv) {
    bool captureAndExit = (argc > 1 && std::string(argv[1]) == "--capture");
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

    const int initW = 1600;
    const int initH = 900;
    GLFWwindow* window = glfwCreateWindow(
        initW, initH,
        "延边朝鲜族民俗村·乡村振兴 | OpenGL 3.3 Core Profile (按 H 查看快捷键)",
        nullptr, nullptr
    );
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    App app;
    g_app = &app;
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!app.init(window, initW, initH)) {
        std::cerr << "Failed to initialize Application\n";
        glfwTerminate();
        return -1;
    }

    if (captureAndExit) {
        app.captureComparisonSuite();
        g_app = nullptr;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        app.update(dt);
        app.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_app = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
