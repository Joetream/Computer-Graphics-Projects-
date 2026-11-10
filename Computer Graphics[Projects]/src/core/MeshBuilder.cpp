#include "core/MeshBuilder.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979323846f;

glm::vec3 xformPos(const glm::mat4& M, const glm::vec3& p) {
    return glm::vec3(M * glm::vec4(p, 1.0f));
}

glm::vec3 xformNorm(const glm::mat3& N, const glm::vec3& n) {
    glm::vec3 v = N * n;
    float len = glm::length(v);
    return len > 1e-6f ? (v / len) : glm::vec3(0.0f, 1.0f, 0.0f);
}
} // namespace

void MeshBuilder::clear() {
    verts.clear();
    indices.clear();
}

void MeshBuilder::addBox(const glm::mat4& M, const glm::vec3& linearColor) {
    glm::mat3 N = glm::inverseTranspose(glm::mat3(M));
    struct Face {
        glm::vec3 n;
        glm::vec3 p[4];
    };
    const Face faces[6] = {
        { { 0,  0,  1}, {{-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}} },
        { { 0,  0, -1}, {{ 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}} },
        { {-1,  0,  0}, {{-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}} },
        { { 1,  0,  0}, {{ 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}} },
        { { 0,  1,  0}, {{-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}} },
        { { 0, -1,  0}, {{-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}} },
    };
    const glm::vec2 uvs[4] = {{0,0}, {1,0}, {1,1}, {0,1}};

    for (const auto& f : faces) {
        uint32_t base = static_cast<uint32_t>(verts.size());
        glm::vec3 wn = xformNorm(N, f.n);
        for (int i = 0; i < 4; ++i) {
            verts.push_back({xformPos(M, f.p[i]), wn, uvs[i], linearColor});
        }
        indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
}

void MeshBuilder::addCylinder(const glm::mat4& M, float r0, float r1, float h, int seg, const glm::vec3& linearColor) {
    if (seg < 3) seg = 3;
    glm::mat3 N = glm::inverseTranspose(glm::mat3(M));
    float slopeY = (r0 - r1) / (h > 1e-5f ? h : 1.0f);

    uint32_t sideBase = static_cast<uint32_t>(verts.size());
    for (int i = 0; i <= seg; ++i) {
        float u = static_cast<float>(i) / seg;
        float ang = u * 2.0f * PI;
        float c = std::cos(ang), s = std::sin(ang);
        glm::vec3 n = glm::normalize(glm::vec3(c, slopeY, s));
        glm::vec3 wn = xformNorm(N, n);

        verts.push_back({xformPos(M, glm::vec3(r0 * c, 0.0f, r0 * s)), wn, {u, 0.0f}, linearColor});
        verts.push_back({xformPos(M, glm::vec3(r1 * c, h,    r1 * s)), wn, {u, 1.0f}, linearColor});
    }
    for (int i = 0; i < seg; ++i) {
        uint32_t k = sideBase + i * 2;
        indices.insert(indices.end(), {k, k + 2, k + 1, k + 1, k + 2, k + 3});
    }

    // Top cap
    if (r1 > 1e-5f) {
        uint32_t centerIdx = static_cast<uint32_t>(verts.size());
        glm::vec3 upN = xformNorm(N, glm::vec3(0, 1, 0));
        verts.push_back({xformPos(M, glm::vec3(0, h, 0)), upN, {0.5f, 0.5f}, linearColor});
        for (int i = 0; i <= seg; ++i) {
            float ang = (static_cast<float>(i) / seg) * 2.0f * PI;
            float c = std::cos(ang), s = std::sin(ang);
            verts.push_back({xformPos(M, glm::vec3(r1 * c, h, r1 * s)), upN, {0.5f + 0.5f * c, 0.5f + 0.5f * s}, linearColor});
        }
        for (int i = 0; i < seg; ++i) {
            indices.insert(indices.end(), {centerIdx, centerIdx + 1 + i + 1, centerIdx + 1 + i});
        }
    }

    // Bottom cap
    if (r0 > 1e-5f) {
        uint32_t centerIdx = static_cast<uint32_t>(verts.size());
        glm::vec3 dnN = xformNorm(N, glm::vec3(0, -1, 0));
        verts.push_back({xformPos(M, glm::vec3(0, 0, 0)), dnN, {0.5f, 0.5f}, linearColor});
        for (int i = 0; i <= seg; ++i) {
            float ang = (static_cast<float>(i) / seg) * 2.0f * PI;
            float c = std::cos(ang), s = std::sin(ang);
            verts.push_back({xformPos(M, glm::vec3(r0 * c, 0, r0 * s)), dnN, {0.5f + 0.5f * c, 0.5f + 0.5f * s}, linearColor});
        }
        for (int i = 0; i < seg; ++i) {
            indices.insert(indices.end(), {centerIdx, centerIdx + 1 + i, centerIdx + 1 + i + 1});
        }
    }
}

void MeshBuilder::addQuad(const glm::mat4& M, const glm::vec3& linearColor) {
    glm::mat3 N = glm::inverseTranspose(glm::mat3(M));
    glm::vec3 wn = xformNorm(N, glm::vec3(0, 1, 0));
    uint32_t base = static_cast<uint32_t>(verts.size());
    verts.push_back({xformPos(M, {-0.5f, 0.0f,  0.5f}), wn, {0.0f, 0.0f}, linearColor});
    verts.push_back({xformPos(M, { 0.5f, 0.0f,  0.5f}), wn, {1.0f, 0.0f}, linearColor});
    verts.push_back({xformPos(M, { 0.5f, 0.0f, -0.5f}), wn, {1.0f, 1.0f}, linearColor});
    verts.push_back({xformPos(M, {-0.5f, 0.0f, -0.5f}), wn, {0.0f, 1.0f}, linearColor});
    indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void MeshBuilder::addGrid(float w, float d, int nx, int nz, const glm::vec3& linearColor) {
    uint32_t base = static_cast<uint32_t>(verts.size());
    for (int iz = 0; iz <= nz; ++iz) {
        float vz = static_cast<float>(iz) / nz;
        float z = (vz - 0.5f) * d;
        for (int ix = 0; ix <= nx; ++ix) {
            float vx = static_cast<float>(ix) / nx;
            float x = (vx - 0.5f) * w;
            verts.push_back({{x, 0.0f, z}, {0.0f, 1.0f, 0.0f}, {vx, vz}, linearColor});
        }
    }
    int stride = nx + 1;
    for (int iz = 0; iz < nz; ++iz) {
        for (int ix = 0; ix < nx; ++ix) {
            uint32_t i0 = base + iz * stride + ix;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + stride;
            uint32_t i3 = i2 + 1;
            indices.insert(indices.end(), {i0, i2, i1, i1, i2, i3});
        }
    }
}

void MeshBuilder::addTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                              const glm::vec3& linearColor, const glm::mat4& M) {
    glm::vec3 wp0 = xformPos(M, p0);
    glm::vec3 wp1 = xformPos(M, p1);
    glm::vec3 wp2 = xformPos(M, p2);
    glm::vec3 n = glm::cross(wp1 - wp0, wp2 - wp0);
    float len = glm::length(n);
    n = len > 1e-6f ? (n / len) : glm::vec3(0, 1, 0);

    uint32_t base = static_cast<uint32_t>(verts.size());
    verts.push_back({wp0, n, {0.0f, 0.0f}, linearColor});
    verts.push_back({wp1, n, {1.0f, 0.0f}, linearColor});
    verts.push_back({wp2, n, {0.5f, 1.0f}, linearColor});
    indices.insert(indices.end(), {base, base + 1, base + 2});
}

void MeshBuilder::append(const MeshBuilder& other, const glm::mat4& M) {
    glm::mat3 N = glm::inverseTranspose(glm::mat3(M));
    uint32_t base = static_cast<uint32_t>(verts.size());
    verts.reserve(verts.size() + other.verts.size());
    for (const auto& v : other.verts) {
        verts.push_back({xformPos(M, v.pos), xformNorm(N, v.normal), v.uv, v.color});
    }
    indices.reserve(indices.size() + other.indices.size());
    for (uint32_t idx : other.indices) {
        indices.push_back(base + idx);
    }
}

void MeshBuilder::build(Mesh& outMesh) const {
    outMesh.upload(verts, indices);
}
