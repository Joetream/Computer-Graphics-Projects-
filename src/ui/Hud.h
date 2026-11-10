#pragma once
#include "core/Shader.h"
#include "core/Texture.h"

class Hud {
public:
    Hud() = default;
    ~Hud();

    void init();
    // Draw a 2D texture in normalized device coordinates [ndcX0, ndcY0] .. [ndcX1, ndcY1]
    void drawQuad(const Shader& hudShader, const Texture& tex,
                  float ndcX0, float ndcY0, float ndcX1, float ndcY1,
                  bool showPixelGrid = false) const;

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
};
