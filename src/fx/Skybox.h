#pragma once
#include "core/Shader.h"

class Texture;

class Skybox {
public:
    Skybox() = default;
    ~Skybox();

    void init();
    void draw(const Shader& skyboxShader, unsigned int cubemapA, unsigned int cubemapB, float blendFactor) const;

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
};
