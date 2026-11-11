#include "scene/KoreanHouse.h"

#include "scene/BezierRoof.h"
#include "core/MeshBuilder.h"
#include "math/MathUtil.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// ============================================================================
// WP-3 · KoreanHouse.cpp
// 朝鲜族民居 / 迎宾门楼 / 广场道具的程序化建模（全部合并为静态大网格）。
//   · buildAllHouses  ：石砌台基 -> 暖白墙体 -> 深褐木构（立柱/横梁/格栅门窗）
//                       -> 青灰歇山顶（BezierRoof）-> 屋旁独立火炕烟囱，
//                       烟囱顶端世界坐标导出给粒子系统（炊烟发射点）。
//   · buildWelcomeGate：村口双柱歇山顶门楼 + 正中竖直悬挂的横匾 Quad（sign_gate.png）。
//   · buildProps      ：百年石砌老水井（八角井圈+小歇山井亭+辘轳）、
//                       2D 村落沙盘导览木台（台面 Quad 贴 Raster2DBoard 动态纹理）、
//                       村口旅游导览立牌木框（正面 Quad 贴 sign_guide.png）。
// 调色板统一经 MathUtil::srgbToLinear 转线性空间。
// ============================================================================

namespace {

// ---- 统一调色板（sRGB -> 线性空间）----
const glm::vec3 kWallColor   = MathUtil::srgbToLinear(0xF2EFE8); // 暖白灰墙
const glm::vec3 kTileColor   = MathUtil::srgbToLinear(0x4A5560); // 青灰筒瓦
const glm::vec3 kWoodColor   = MathUtil::srgbToLinear(0x5B3A29); // 深褐木梁柱、格栅
const glm::vec3 kStoneColor  = MathUtil::srgbToLinear(0x9A958C); // 石砌台基、老水井
const glm::vec3 kBrickColor  = MathUtil::srgbToLinear(0xC85A48); // 烟囱红褐砖石
const glm::vec3 kAzaleaColor = MathUtil::srgbToLinear(0xE07AA0); // 金达莱粉紫点缀
const glm::vec3 kWhiteColor  = MathUtil::srgbToLinear(0xE6E9EC); // 贴图 Quad 底色

// 平移*缩放 的常用组合：在 (x,y,z) 处放置尺寸 (sx,sy,sz) 的盒体
glm::mat4 boxAt(float x, float y, float z, float sx, float sy, float sz) {
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z))
         * glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
}
glm::mat4 translateTo(float x, float y, float z) {
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
}
glm::mat4 rotateXDeg(float deg) {
    return glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(1.0f, 0.0f, 0.0f));
}
glm::mat4 rotateZDeg(float deg) {
    return glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(0.0f, 0.0f, 1.0f));
}

// 独立烟囱顶端（房屋局部坐标）：筒身顶圈之上、遮雨小帽之下，炊烟发射点
const glm::vec3 kChimneyTopLocal(4.5f, 4.46f, -1.2f);

// ---------------------------------------------------------------------------
// 单栋民居几何（局部坐标：原点在房屋中心地面，+Z 为正面）
// ---------------------------------------------------------------------------
void buildHouseGeometry(MeshBuilder& b, int roofSteps) {
    // 1. 石砌台基：宽 7.6、深 5.2、高 0.55
    b.addBox(boxAt(0.0f, 0.275f, 0.0f, 7.6f, 0.55f, 5.2f), kStoneColor);

    // 2. 暖白墙体：宽 6.8、深 4.4、高 2.6（y 0.55 -> 3.15）
    b.addBox(boxAt(0.0f, 1.85f, 0.0f, 6.8f, 2.6f, 4.4f), kWallColor);

    // 3. 深褐木构：四角立柱 + 檐下横梁
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.addBox(boxAt(3.26f * sx, 1.85f, 2.06f * sz, 0.28f, 2.6f, 0.28f), kWoodColor);
        }
        b.addBox(boxAt(3.26f * sx, 3.02f, 0.0f, 0.20f, 0.24f, 4.50f), kWoodColor); // 侧檐梁
    }
    b.addBox(boxAt(0.0f, 3.02f, 2.06f, 6.90f, 0.24f, 0.20f), kWoodColor);          // 前檐梁
    b.addBox(boxAt(0.0f, 3.02f, -2.06f, 6.90f, 0.24f, 0.20f), kWoodColor);         // 后檐梁

    // 门前石阶
    b.addBox(boxAt(0.0f, 0.10f, 2.85f, 2.30f, 0.20f, 1.10f), kStoneColor);

    // 正面韩式木格栅推拉门（x -1.25..1.25，y 0.62..2.90，z≈2.24）
    b.addBox(boxAt(-1.25f, 1.76f, 2.24f, 0.12f, 2.28f, 0.10f), kWoodColor);        // 门边梃
    b.addBox(boxAt(1.25f, 1.76f, 2.24f, 0.12f, 2.28f, 0.10f), kWoodColor);
    b.addBox(boxAt(0.0f, 2.84f, 2.24f, 2.62f, 0.12f, 0.10f), kWoodColor);          // 上槛
    b.addBox(boxAt(0.0f, 0.68f, 2.24f, 2.62f, 0.12f, 0.10f), kWoodColor);          // 下槛
    b.addBox(boxAt(0.0f, 1.78f, 2.25f, 2.42f, 0.09f, 0.08f), kWoodColor);          // 中横档
    for (int i = 0; i <= 6; ++i) {                                                 // 竖棂
        const float x = -1.05f + 0.35f * float(i);
        b.addBox(boxAt(x, 1.78f, 2.27f, 0.05f, 2.02f, 0.045f), kWoodColor);
    }
    for (int i = 0; i <= 4; ++i) {                                                 // 横棂
        const float y = 0.92f + 0.43f * float(i);
        b.addBox(boxAt(0.0f, y, 2.28f, 2.32f, 0.045f, 0.045f), kWoodColor);
    }

    // 两侧木格栅窗（x=±3.42，z -0.8..0.8，y 1.35..2.35）
    for (int sideX = -1; sideX <= 1; sideX += 2) {
        const float wx = 3.42f * float(sideX);
        b.addBox(boxAt(wx, 1.85f, -0.80f, 0.08f, 1.00f, 0.10f), kWoodColor);       // 窗边梃
        b.addBox(boxAt(wx, 1.85f, 0.80f, 0.08f, 1.00f, 0.10f), kWoodColor);
        b.addBox(boxAt(wx, 1.35f, 0.0f, 0.08f, 0.10f, 1.70f), kWoodColor);         // 窗下槛
        b.addBox(boxAt(wx, 2.35f, 0.0f, 0.08f, 0.10f, 1.70f), kWoodColor);         // 窗上槛
        for (int i = 0; i <= 4; ++i) {                                             // 竖棂
            const float z = -0.64f + 0.32f * float(i);
            b.addBox(boxAt(wx + 0.01f * sideX, 1.85f, z, 0.045f, 0.92f, 0.05f), kWoodColor);
        }
        for (int i = 1; i <= 2; ++i) {                                             // 横棂
            const float y = 1.35f + 0.35f * float(i) + 0.15f * float(i);           // 1.85 / 2.20
            b.addBox(boxAt(wx + 0.01f * sideX, y, 0.0f, 0.04f, 0.045f, 1.54f), kWoodColor);
        }
    }

    // 4. 青灰歇山顶：宽 8.4、深 6.0、高 2.1、起翘 0.55（檐口基面 = 墙顶 3.15）
    BezierRoof roof;
    roof.build(8.4f, 6.0f, 2.1f, 0.55f, roofSteps, b,
               translateTo(0.0f, 3.15f, 0.0f));

    // [1.0reborn] 封檐垫板：墙顶 (y=3.15) 与屋面下表面之间原有 0.59~1.03 m 空隙，
    // 沿墙体四周（半宽 3.4、半深 2.2）补一圈暖白墙板，板顶贴合屋面网格
    BezierRoof::buildEaveSeal(8.4f, 6.0f, 2.1f, 0.55f, roofSteps, 3.4f, 2.2f, kWallColor, b,
                              translateTo(0.0f, 3.15f, 0.0f));
}

// ---------------------------------------------------------------------------
// 文化标志——屋旁独立火炕烟囱（房屋局部坐标 (4.5, 0, -1.2)，与地下火炕相连）
// ---------------------------------------------------------------------------
void buildChimney(MeshBuilder& b) {
    const float cx = 4.5f;
    const float cz = -1.2f;
    b.addBox(boxAt(cx, 0.28f, cz, 0.95f, 0.56f, 0.95f), kBrickColor);              // 方形底座
    b.addCylinder(translateTo(cx, 0.50f, cz), 0.24f, 0.20f, 3.8f, 12, kBrickColor);// 高 3.8 圆筒烟囱
    b.addCylinder(translateTo(cx, 4.30f, cz), 0.27f, 0.27f, 0.10f, 12, kBrickColor);// 顶圈
    b.addBox(boxAt(cx - 0.16f, 4.44f, cz, 0.07f, 0.12f, 0.07f), kWoodColor);       // 小帽支脚
    b.addBox(boxAt(cx + 0.16f, 4.44f, cz, 0.07f, 0.12f, 0.07f), kWoodColor);
    b.addBox(boxAt(cx, 4.55f, cz, 0.70f, 0.08f, 0.70f), kWoodColor);               // 遮雨小帽
}

} // namespace

// ---------------------------------------------------------------------------
// 遍历所有民居实例，合并上传至 outVillageMesh，并导出烟囱顶端世界坐标
// ---------------------------------------------------------------------------
void KoreanVillageBuilder::buildAllHouses(const std::vector<HouseInstance>& houses, int bezierLevel,
                                          Mesh& outVillageMesh, std::vector<glm::vec3>& outChimneyTops) {
    outChimneyTops.clear();
    MeshBuilder village;
    const int roofSteps = BezierRoof::stepsFromLevel(bezierLevel);

    for (const HouseInstance& h : houses) {
        MeshBuilder house;
        buildHouseGeometry(house, roofSteps);
        if (h.hasChimney) buildChimney(house);

        const glm::mat4 model = glm::translate(glm::mat4(1.0f), h.pos)
                              * glm::rotate(glm::mat4(1.0f), glm::radians(h.yawDeg), glm::vec3(0.0f, 1.0f, 0.0f))
                              * glm::scale(glm::mat4(1.0f), h.scale);
        village.append(house, model);

        if (h.hasChimney) {
            outChimneyTops.push_back(glm::vec3(model * glm::vec4(kChimneyTopLocal, 1.0f)));
        }
    }
    village.build(outVillageMesh);
}

// ---------------------------------------------------------------------------
// 村口迎宾门楼（双柱四阿/歇山顶，高 5.8 m，跨度 8.5 m）+ 正中竖直横匾
// ---------------------------------------------------------------------------
void KoreanVillageBuilder::buildWelcomeGate(int bezierLevel, Mesh& outGateMesh,
                                            Mesh& outPlaqueQuad, glm::mat4& outPlaqueModel) {
    MeshBuilder gate;
    const float gz = 32.0f;

    // 两侧石砌基座与门槛（中央留出通行开口）
    gate.addBox(boxAt(-5.15f, 0.20f, gz, 1.90f, 0.40f, 3.20f), kStoneColor);
    gate.addBox(boxAt(5.15f, 0.20f, gz, 1.90f, 0.40f, 3.20f), kStoneColor);
    gate.addBox(boxAt(0.0f, 0.05f, gz, 8.60f, 0.10f, 2.60f), kStoneColor);

    // 双柱（外缘 ±4.25 -> 跨度 8.5 m）、柱础与雀替
    for (int s = -1; s <= 1; s += 2) {
        const float x = 3.90f * float(s);
        gate.addBox(boxAt(x, 0.25f, gz, 1.05f, 0.50f, 1.05f), kStoneColor);        // 柱础
        gate.addBox(boxAt(x, 1.80f, gz, 0.70f, 3.60f, 0.70f), kWoodColor);         // 柱身 0..3.6
        gate.addBox(boxAt(x, 3.72f, gz, 1.35f, 0.28f, 0.95f), kWoodColor);         // 雀替
    }

    // 双重横梁 + 斗拱块
    gate.addBox(boxAt(0.0f, 3.98f, gz, 8.90f, 0.34f, 0.72f), kWoodColor);
    gate.addBox(boxAt(0.0f, 4.28f, gz, 9.30f, 0.26f, 0.82f), kWoodColor);
    for (int i = -2; i <= 2; ++i) {
        gate.addBox(boxAt(1.60f * float(i), 4.51f, gz, 0.34f, 0.26f, 0.58f), kWoodColor);
    }

    // 横匾吊带（自横梁底垂至匾顶）
    gate.addBox(boxAt(-2.00f, 3.66f, gz + 0.60f, 0.06f, 0.32f, 0.04f), kWoodColor);
    gate.addBox(boxAt(2.00f, 3.66f, gz + 0.60f, 0.06f, 0.32f, 0.04f), kWoodColor);

    // 歇山顶门楼帽：基面 4.55，脊高 5.80
    BezierRoof roof;
    roof.build(9.8f, 5.2f, 1.25f, 0.48f, BezierRoof::stepsFromLevel(bezierLevel), gate,
               translateTo(0.0f, 4.55f, gz));
    gate.build(outGateMesh);

    // 门楼正中央竖直悬挂的横匾：宽 4.8、高 1.2，法线朝 +Z，UV 对应整张 sign_gate.png
    MeshBuilder plaque;
    const glm::mat4 quadM = rotateXDeg(90.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(4.8f, 1.0f, 1.2f));
    plaque.addQuad(quadM, kWhiteColor);
    plaque.build(outPlaqueQuad);
    outPlaqueModel = translateTo(0.0f, 2.90f, gz + 0.62f);
}

// ---------------------------------------------------------------------------
// 广场与庭院道具：老水井 / 2D 沙盘导览木台 / 旅游导览立牌
// ---------------------------------------------------------------------------
void KoreanVillageBuilder::buildProps(Mesh& outPropsMesh, Mesh& outSandboxTopQuad, glm::mat4& outSandboxTopModel,
                                      Mesh& outGuideSignQuad, glm::mat4& outGuideSignModel) {
    MeshBuilder props;
    const int roofSteps = BezierRoof::stepsFromLevel(1); // 道具小顶固定中等细分

    // ---- 1) 百年石砌老水井 (3, 0, 0)：八角石井圈 + 双木柱小歇山井亭 + 辘轳横轴 ----
    props.addCylinder(translateTo(3.0f, 0.0f, 0.0f), 0.62f, 0.55f, 0.55f, 8, kStoneColor); // 八角井圈
    props.addQuad(translateTo(3.0f, 0.562f, 0.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.86f, 1.0f, 0.86f)),
                  kTileColor);                                                             // 井口水墨面
    for (int s = -1; s <= 1; s += 2) {
        props.addBox(boxAt(3.0f + 0.78f * float(s), 1.05f, 0.0f, 0.14f, 2.10f, 0.14f), kWoodColor); // 双木柱
    }
    BezierRoof wellRoof;
    wellRoof.build(2.5f, 2.1f, 0.62f, 0.16f, roofSteps, props, translateTo(3.0f, 2.12f, 0.0f)); // 小歇山井亭
    props.addCylinder(translateTo(3.0f, 1.75f, 0.0f) * rotateZDeg(90.0f) * translateTo(0.0f, -0.72f, 0.0f),
                      0.07f, 0.07f, 1.44f, 8, kWoodColor);                                   // 辘轳横轴
    props.addBox(boxAt(3.78f, 1.75f, 0.0f, 0.26f, 0.05f, 0.05f), kWoodColor);               // 摇把臂
    props.addBox(boxAt(3.89f, 1.62f, 0.0f, 0.05f, 0.30f, 0.05f), kWoodColor);               // 摇把柄
    props.addCylinder(translateTo(3.0f, 0.60f, 0.0f), 0.018f, 0.018f, 1.15f, 6, kWoodColor); // 井绳
    props.addBox(boxAt(3.0f, 0.53f, 0.0f, 0.20f, 0.15f, 0.20f), kWoodColor);                // 吊桶

    // ---- 2) 2D 村落沙盘导览木台 (-7.5, 0, 18) ----
    const float sx = -7.5f;
    const float sz = 18.0f;
    for (int ix = -1; ix <= 1; ix += 2) {
        for (int iz = -1; iz <= 1; iz += 2) {
            props.addBox(boxAt(sx + 0.86f * float(ix), 0.36f, sz + 0.62f * float(iz), 0.12f, 0.72f, 0.12f),
                         kWoodColor);                                                      // 四腿
        }
    }
    props.addBox(boxAt(sx, 0.78f, sz, 2.00f, 0.12f, 1.50f), kWoodColor);                   // 桌身
    props.addBox(boxAt(sx, 0.90f, sz - 0.715f, 2.00f, 0.10f, 0.07f), kWoodColor);          // 边框
    props.addBox(boxAt(sx, 0.90f, sz + 0.715f, 2.00f, 0.10f, 0.07f), kWoodColor);
    props.addBox(boxAt(sx - 0.965f, 0.90f, sz, 0.07f, 0.10f, 1.50f), kWoodColor);
    props.addBox(boxAt(sx + 0.965f, 0.90f, sz, 0.07f, 0.10f, 1.50f), kWoodColor);
    // 台面上的迷你民居与水井标记
    props.addBox(boxAt(sx - 0.45f, 0.93f, sz - 0.25f, 0.26f, 0.10f, 0.18f), kWallColor);
    props.addBox(boxAt(sx - 0.45f, 1.00f, sz - 0.25f, 0.30f, 0.06f, 0.22f), kTileColor);
    props.addBox(boxAt(sx + 0.30f, 0.93f, sz + 0.20f, 0.22f, 0.10f, 0.16f), kWallColor);
    props.addBox(boxAt(sx + 0.30f, 1.00f, sz + 0.20f, 0.26f, 0.06f, 0.20f), kTileColor);
    props.addCylinder(translateTo(sx + 0.55f, 0.90f, sz - 0.30f), 0.07f, 0.06f, 0.09f, 8, kStoneColor);

    // 台面水平 Quad：单元四边形，世界变换由 outSandboxTopModel 给出（贴 Raster2DBoard 动态纹理）
    MeshBuilder sandboxQuad;
    sandboxQuad.addQuad(glm::mat4(1.0f), kWhiteColor);
    sandboxQuad.build(outSandboxTopQuad);
    outSandboxTopModel = translateTo(sx, 0.905f, sz)
                       * glm::scale(glm::mat4(1.0f), glm::vec3(1.84f, 1.0f, 1.34f));

    // ---- 3) 村口旅游导览立牌木框 (7.5, 0, 22)：双柱木框 + 顶帽 + 金达莱花坛 ----
    const float px = 7.5f;
    const float pz = 22.0f;
    for (int s = -1; s <= 1; s += 2) {
        props.addBox(boxAt(px + 0.82f * float(s), 1.10f, pz, 0.12f, 2.20f, 0.12f), kWoodColor); // 双柱
    }
    props.addBox(boxAt(px, 2.14f, pz, 1.80f, 0.12f, 0.09f), kWoodColor);                    // 上框
    props.addBox(boxAt(px, 0.96f, pz, 1.80f, 0.12f, 0.09f), kWoodColor);                    // 下框
    props.addBox(boxAt(px - 0.78f, 1.55f, pz, 0.10f, 1.30f, 0.09f), kWoodColor);            // 侧框
    props.addBox(boxAt(px + 0.78f, 1.55f, pz, 0.10f, 1.30f, 0.09f), kWoodColor);
    props.addBox(translateTo(px, 2.30f, pz + 0.05f) * rotateXDeg(10.0f)
                       * glm::scale(glm::mat4(1.0f), glm::vec3(1.90f, 0.06f, 0.55f)), kTileColor); // 顶帽
    props.addBox(boxAt(px - 1.30f, 0.20f, pz + 0.35f, 0.42f, 0.40f, 0.42f), kBrickColor);   // 花坛
    props.addBox(boxAt(px + 1.30f, 0.20f, pz + 0.35f, 0.42f, 0.40f, 0.42f), kBrickColor);
    props.addCylinder(translateTo(px - 1.30f, 0.40f, pz + 0.35f), 0.24f, 0.18f, 0.30f, 8, kAzaleaColor); // 金达莱
    props.addCylinder(translateTo(px + 1.30f, 0.40f, pz + 0.35f), 0.24f, 0.18f, 0.30f, 8, kAzaleaColor);

    // 立牌正面竖直 Quad（sign_guide.png 为 512x512 方形 -> 1.02 x 1.02）
    MeshBuilder signQuad;
    const glm::mat4 signM = rotateXDeg(90.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(1.02f, 1.0f, 1.02f));
    signQuad.addQuad(signM, kWhiteColor);
    signQuad.build(outGuideSignQuad);
    outGuideSignModel = translateTo(px, 1.55f, pz + 0.07f);

    props.build(outPropsMesh);
}
