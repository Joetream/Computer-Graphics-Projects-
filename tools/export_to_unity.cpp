// ============================================================================
// WP-U1 · tools/export_to_unity.cpp
// C++17 -> Unity 3D 资产桥接导出器：
// 调用 1.0 引擎的 BezierPatch / BezierRoof / MeshBuilder，将双三次 Bézier C1 飞檐屋顶、
// 朝鲜族传统民居、四柱迎宾牌楼及百年水井导出为标准 Wavefront .obj 网格文件，
// 供 Computer Graphics2.0[Project_unity] 直接检视与加载。
// ============================================================================
#include "core/MeshBuilder.h"
#include "math/BezierPatch.h"
#include "math/MathUtil.h"
#include "scene/BezierRoof.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

// Dummy Mesh::upload for headless CLI exporter (no OpenGL context needed)
Mesh::~Mesh() {}
Mesh::Mesh(Mesh&& other) noexcept {}
Mesh& Mesh::operator=(Mesh&& other) noexcept { return *this; }
void Mesh::upload(const std::vector<Vertex>&, const std::vector<uint32_t>&) {}
void Mesh::setInstances(const std::vector<glm::mat4>&) {}
void Mesh::draw() const {}
void Mesh::drawInstanced(int) const {}
void Mesh::ensureBuffers() {}

namespace {

bool writeObjFile(const std::string& path, const std::string& objName, const MeshBuilder& mb) {
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "[Exporter] Cannot open for write: " << path << "\n";
        return false;
    }
    out << "# Exported by YanbianVillage C++17 -> Unity 2.0 Asset Bridge\n";
    out << "# Object: " << objName << " (Vertices: " << mb.verts.size()
        << ", Triangles: " << (mb.indices.size() / 3) << ")\n";
    out << "o " << objName << "\n";
    out << std::fixed << std::setprecision(5);

    // Note: Convert right-handed OpenGL (+X, +Y, +Z) to Unity left-handed (-X, +Y, +Z)
    for (const auto& v : mb.verts) {
        out << "v " << -v.pos.x << " " << v.pos.y << " " << v.pos.z << " "
            << v.color.r << " " << v.color.g << " " << v.color.b << "\n";
    }
    for (const auto& v : mb.verts) {
        out << "vt " << v.uv.x << " " << v.uv.y << "\n";
    }
    for (const auto& v : mb.verts) {
        out << "vn " << -v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
    }
    // Reverse triangle winding (i0, i2, i1) to match X-flip handedness conversion
    for (size_t i = 0; i + 2 < mb.indices.size(); i += 3) {
        uint32_t i0 = mb.indices[i + 0] + 1;
        uint32_t i1 = mb.indices[i + 2] + 1;
        uint32_t i2 = mb.indices[i + 1] + 1;
        out << "f " << i0 << "/" << i0 << "/" << i0 << " "
                    << i1 << "/" << i1 << "/" << i1 << " "
                    << i2 << "/" << i2 << "/" << i2 << "\n";
    }
    std::cout << "[Exporter] Wrote " << path << " (" << mb.verts.size()
              << " verts, " << (mb.indices.size() / 3) << " tris)\n";
    return true;
}

glm::mat4 boxAt(float x, float y, float z, float sx, float sy, float sz) {
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z))
         * glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
}

glm::mat4 translateTo(float x, float y, float z) {
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
}

} // namespace

int main(int argc, char** argv) {
    std::string outDir = "Computer Graphics2.0[Project_unity]/Assets/Meshes_Exported";
    if (argc > 1) outDir = argv[1];

    const glm::vec3 kWallColor  = MathUtil::srgbToLinear(0xF2EFE8);
    const glm::vec3 kTileColor  = MathUtil::srgbToLinear(0x4A5560);
    const glm::vec3 kWoodColor  = MathUtil::srgbToLinear(0x5B3A29);
    const glm::vec3 kStoneColor = MathUtil::srgbToLinear(0x9A958C);
    const glm::vec3 kBrickColor = MathUtil::srgbToLinear(0xC85A48);

    // 1. Export Single Korean House with 6-Patch Bicubic Bezier Roof & Standalone Chimney
    {
        MeshBuilder house;
        house.addBox(boxAt(0.0f, 0.275f, 0.0f, 7.6f, 0.55f, 5.2f), kStoneColor);
        house.addBox(boxAt(0.0f, 1.85f, 0.0f, 6.8f, 2.6f, 4.4f), kWallColor);
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                house.addBox(boxAt(3.26f * sx, 1.85f, 2.06f * sz, 0.28f, 2.6f, 0.28f), kWoodColor);
            }
            house.addBox(boxAt(3.26f * sx, 3.02f, 0.0f, 0.20f, 0.24f, 4.50f), kWoodColor);
        }
        house.addBox(boxAt(0.0f, 3.02f, 2.06f, 6.90f, 0.24f, 0.20f), kWoodColor);
        house.addBox(boxAt(0.0f, 3.02f, -2.06f, 6.90f, 0.24f, 0.20f), kWoodColor);
        house.addBox(boxAt(0.0f, 0.10f, 2.85f, 2.30f, 0.20f, 1.10f), kStoneColor);

        BezierRoof roof;
        roof.build(8.4f, 6.0f, 2.1f, 0.55f, 1, house, translateTo(0.0f, 3.15f, 0.0f));

        // Standalone Ondol Chimney
        const float cx = 4.5f, cz = -1.2f;
        house.addBox(boxAt(cx, 0.28f, cz, 0.95f, 0.56f, 0.95f), kBrickColor);
        house.addCylinder(translateTo(cx, 0.50f, cz), 0.24f, 0.20f, 3.8f, 12, kBrickColor);
        house.addCylinder(translateTo(cx, 4.30f, cz), 0.27f, 0.27f, 0.10f, 12, kBrickColor);
        house.addBox(boxAt(cx, 4.55f, cz, 0.70f, 0.08f, 0.70f), kWoodColor);

        writeObjFile(outDir + "/KoreanHouse_BezierRoof.obj", "KoreanHouse_BezierRoof", house);
    }

    // 2. Export Welcome Gate with Bicubic Bezier Roof
    {
        MeshBuilder gate;
        gate.addBox(boxAt(-5.15f, 0.20f, 0.0f, 1.90f, 0.40f, 3.20f), kStoneColor);
        gate.addBox(boxAt(5.15f, 0.20f, 0.0f, 1.90f, 0.40f, 3.20f), kStoneColor);
        gate.addBox(boxAt(0.0f, 0.05f, 0.0f, 8.60f, 0.10f, 2.60f), kStoneColor);
        for (int s = -1; s <= 1; s += 2) {
            const float x = 3.90f * float(s);
            gate.addBox(boxAt(x, 0.25f, 0.0f, 1.05f, 0.50f, 1.05f), kStoneColor);
            gate.addBox(boxAt(x, 1.80f, 0.0f, 0.70f, 3.60f, 0.70f), kWoodColor);
            gate.addBox(boxAt(x, 3.72f, 0.0f, 1.35f, 0.28f, 0.95f), kWoodColor);
        }
        gate.addBox(boxAt(0.0f, 3.98f, 0.0f, 8.90f, 0.34f, 0.72f), kWoodColor);
        gate.addBox(boxAt(0.0f, 4.28f, 0.0f, 9.30f, 0.26f, 0.82f), kWoodColor);
        BezierRoof roof;
        roof.build(9.8f, 5.2f, 1.25f, 0.48f, 1, gate, translateTo(0.0f, 4.55f, 0.0f));
        writeObjFile(outDir + "/WelcomeGate.obj", "WelcomeGate", gate);
    }

    // 3. Export Traditional Stone Well with Pavilion Roof
    {
        MeshBuilder well;
        well.addCylinder(translateTo(0.0f, 0.0f, 0.0f), 0.62f, 0.55f, 0.55f, 8, kStoneColor);
        for (int s = -1; s <= 1; s += 2) {
            well.addBox(boxAt(0.78f * float(s), 1.05f, 0.0f, 0.14f, 2.10f, 0.14f), kWoodColor);
        }
        BezierRoof wellRoof;
        wellRoof.build(2.5f, 2.1f, 0.62f, 0.16f, 1, well, translateTo(0.0f, 2.12f, 0.0f));
        writeObjFile(outDir + "/VillageProps.obj", "VillageProps_StoneWell", well);
    }

    return 0;
}
