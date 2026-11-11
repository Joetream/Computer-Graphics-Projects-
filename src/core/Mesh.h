#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;     // location = 0: 顶点位置
    glm::vec3 normal;  // location = 1: 法向量
    glm::vec2 uv;      // location = 2: 纹理坐标
    glm::vec3 color;   // location = 3: 线性空间顶点色 (sRGB 已做 pow(c, 2.2))
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
    void setInstances(const std::vector<glm::mat4>& models); // location 4..7, divisor=1
    void draw() const;
    void drawInstanced(int count) const;
    int indexCount() const { return m_indexCount; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    unsigned int m_instanceVbo = 0;
    int m_indexCount = 0;
    size_t m_instanceCapacity = 0;

    void ensureBuffers();
};
