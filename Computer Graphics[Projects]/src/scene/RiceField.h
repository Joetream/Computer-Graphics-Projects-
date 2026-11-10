#pragma once
#include "core/Mesh.h"
#include "core/Shader.h"
#include "math/Frustum.h"
#include <vector>
#include <glm/glm.hpp>

class Terrain;

class RiceField {
public:
    void init(const Terrain& terrain);
    void updateVisibleInstances(const Frustum& frustum);
    void draw(const Shader& shader) const;
    int visibleChunks() const { return m_visibleChunks; }
    int totalChunks() const { return static_cast<int>(m_chunks.size()); }

private:
    struct Chunk {
        glm::vec3 minPt;
        glm::vec3 maxPt;
        std::vector<glm::mat4> instances;
    };
    Mesh m_stalkMesh;
    std::vector<Chunk> m_chunks;
    std::vector<glm::mat4> m_visibleBuffer;
    int m_visibleChunks = 0;
    int m_visibleInstanceCount = 0;
};
