#include "core/Mesh.h"
#include <glad/glad.h>
#include <cstddef>
#include <utility>

Mesh::~Mesh() {
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao),
      m_vbo(other.m_vbo),
      m_ebo(other.m_ebo),
      m_instanceVbo(other.m_instanceVbo),
      m_indexCount(other.m_indexCount),
      m_instanceCapacity(other.m_instanceCapacity) {
    other.m_vao = other.m_vbo = other.m_ebo = other.m_instanceVbo = 0;
    other.m_indexCount = 0;
    other.m_instanceCapacity = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
        if (m_ebo) glDeleteBuffers(1, &m_ebo);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);

        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_instanceVbo = other.m_instanceVbo;
        m_indexCount = other.m_indexCount;
        m_instanceCapacity = other.m_instanceCapacity;

        other.m_vao = other.m_vbo = other.m_ebo = other.m_instanceVbo = 0;
        other.m_indexCount = 0;
        other.m_instanceCapacity = 0;
    }
    return *this;
}

void Mesh::ensureBuffers() {
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
    }
}

void Mesh::upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices) {
    ensureBuffers();
    m_indexCount = static_cast<int>(indices.size());

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));

    glBindVertexArray(0);
}

void Mesh::setInstances(const std::vector<glm::mat4>& models) {
    ensureBuffers();
    glBindVertexArray(m_vao);

    if (!m_instanceVbo) {
        glGenBuffers(1, &m_instanceVbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(4 + i);
            glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                                  reinterpret_cast<void*>(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(4 + i, 1);
        }
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    }

    if (models.size() > m_instanceCapacity) {
        m_instanceCapacity = models.size();
        glBufferData(GL_ARRAY_BUFFER, m_instanceCapacity * sizeof(glm::mat4), models.data(), GL_DYNAMIC_DRAW);
    } else if (!models.empty()) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, models.size() * sizeof(glm::mat4), models.data());
    }

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (!m_vao || m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::drawInstanced(int count) const {
    if (!m_vao || m_indexCount == 0 || count <= 0) return;
    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr, count);
    glBindVertexArray(0);
}
