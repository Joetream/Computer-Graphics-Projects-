#include "ui/Hud.h"
#include <glad/glad.h>

Hud::~Hud() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void Hud::init() {
    const float quadVerts[] = {
        // pos(x,y)   uv(u,v)
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,
        0.0f, 1.0f,   0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void Hud::drawQuad(const Shader& hudShader, const Texture& tex,
                   float ndcX0, float ndcY0, float ndcX1, float ndcY1,
                   bool showPixelGrid) const {
    if (!m_vao) return;
    hudShader.use();
    tex.bind(0);
    hudShader.setInt("u_HudTex", 0);
    hudShader.setVec4("u_RectNDC", glm::vec4(ndcX0, ndcY0, ndcX1, ndcY1));
    hudShader.setBool("u_ShowPixelGrid", showPixelGrid);
    hudShader.setFloat("u_GridRes", static_cast<float>(tex.width() > 0 ? tex.width() : 128));

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
