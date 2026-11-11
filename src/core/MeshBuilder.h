#pragma once
#include "core/Mesh.h"
#include <glm/glm.hpp>

class MeshBuilder {
public:
    std::vector<Vertex>   verts;
    std::vector<uint32_t> indices;

    void clear();

    // 所有图元生成函数均将变换矩阵 M 烘焙进顶点位置，并用 transpose(inverse(mat3(M))) 变换法线
    // - addBox:      [-0.5, 0.5]^3 单位立方体
    // - addCylinder: 底部 y=0 (半径 r0) 到顶部 y=h (半径 r1)，分段数 seg
    // - addQuad:     XZ 平面 [-0.5, 0.5]^2，法线朝 +Y
    // - addGrid:     XZ 平面 [-0.5*w, 0.5*w] x [-0.5*d, 0.5*d]，nx * nz 网格
    void addBox(const glm::mat4& M, const glm::vec3& linearColor);
    void addCylinder(const glm::mat4& M, float r0, float r1, float h, int seg, const glm::vec3& linearColor);
    void addQuad(const glm::mat4& M, const glm::vec3& linearColor);
    void addGrid(float w, float d, int nx, int nz, const glm::vec3& linearColor);
    void addTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                     const glm::vec3& linearColor, const glm::mat4& M = glm::mat4(1.0f));
    void append(const MeshBuilder& other, const glm::mat4& M = glm::mat4(1.0f));
    void build(Mesh& outMesh) const;
};
