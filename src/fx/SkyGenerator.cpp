#include "fx/SkyGenerator.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {

constexpr int   kFaceSize = 256;
constexpr float PI        = 3.14159265358979323846f;

struct SkyPalette {
    glm::vec3 zenith;        // 天顶色
    glm::vec3 horizon;       // 地平线色
    glm::vec3 horizonBand;   // 贴地平线的暖/亮色带
    glm::vec3 orbColor;      // 太阳 / 月亮圆盘
    glm::vec3 haloColor;     // 大气光晕
    glm::vec3 mountain;      // 远山剪影
    glm::vec3 snow;          // 山巅积雪
    glm::vec3 ground;        // 下半球（地平线以下）
    glm::vec3 celestialDir;  // 太阳 / 月亮方向（与 LightingState 三态预设一致）
    bool       stars;        // 是否生成繁星
};

float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

float smoothstep(float edge0, float edge1, float x) {
    if (edge1 <= edge0) return x < edge0 ? 0.0f : 1.0f;
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

float hash3(float x, float y, float z) {
    const float h = std::sin(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453123f;
    return h - std::floor(h);
}

float angleDiff(float a, float b) {
    float d = a - b;
    while (d >  PI) d -= 2.0f * PI;
    while (d < -PI) d += 2.0f * PI;
    return d;
}

SkyPalette makePalette(int preset) {
    SkyPalette p{};
    switch (preset) {
    case 1: // 正午晴空
        p.zenith       = glm::vec3(0.16f, 0.40f, 0.76f);
        p.horizon      = glm::vec3(0.70f, 0.84f, 0.95f);
        p.horizonBand  = glm::vec3(0.88f, 0.93f, 0.97f);
        p.orbColor     = glm::vec3(1.00f, 0.99f, 0.92f);
        p.haloColor    = glm::vec3(1.00f, 0.95f, 0.82f);
        p.mountain     = glm::vec3(0.34f, 0.44f, 0.54f);
        p.snow         = glm::vec3(0.96f, 0.98f, 1.00f);
        p.ground       = glm::vec3(0.22f, 0.28f, 0.33f);
        p.celestialDir = glm::normalize(glm::vec3(0.25f, 0.92f, -0.28f));
        p.stars        = false;
        break;
    case 2: // 星夜灯火
        p.zenith       = glm::vec3(0.012f, 0.020f, 0.055f);
        p.horizon      = glm::vec3(0.055f, 0.090f, 0.180f);
        p.horizonBand  = glm::vec3(0.090f, 0.130f, 0.220f);
        p.orbColor     = glm::vec3(0.86f, 0.91f, 1.00f);
        p.haloColor    = glm::vec3(0.30f, 0.42f, 0.72f);
        p.mountain     = glm::vec3(0.025f, 0.038f, 0.075f);
        p.snow         = glm::vec3(0.35f, 0.42f, 0.56f);
        p.ground       = glm::vec3(0.015f, 0.020f, 0.040f);
        p.celestialDir = glm::normalize(glm::vec3(-0.35f, 0.65f, -0.55f));
        p.stars        = true;
        break;
    default: // 0 晨曦初露
        p.zenith       = glm::vec3(0.20f, 0.26f, 0.50f);
        p.horizon      = glm::vec3(0.86f, 0.62f, 0.52f);
        p.horizonBand  = glm::vec3(0.98f, 0.72f, 0.52f);
        p.orbColor     = glm::vec3(1.00f, 0.94f, 0.76f);
        p.haloColor    = glm::vec3(1.00f, 0.62f, 0.36f);
        p.mountain     = glm::vec3(0.30f, 0.26f, 0.36f);
        p.snow         = glm::vec3(0.92f, 0.90f, 0.94f);
        p.ground       = glm::vec3(0.14f, 0.13f, 0.18f);
        p.celestialDir = glm::normalize(glm::vec3(0.75f, 0.26f, -0.45f));
        p.stars        = false;
        break;
    }
    return p;
}

// 标准 cubemap 面方向：纹理坐标 (u, v) ∈ [-1, 1]^2 -> 单位方向向量
glm::vec3 faceDirection(int face, float u, float v) {
    switch (face) {
    case 0: return glm::normalize(glm::vec3( 1.0f, -v, -u)); // +X
    case 1: return glm::normalize(glm::vec3(-1.0f, -v,  u)); // -X
    case 2: return glm::normalize(glm::vec3(     u,  1.0f,  v)); // +Y
    case 3: return glm::normalize(glm::vec3(     u, -1.0f, -v)); // -Y
    case 4: return glm::normalize(glm::vec3(     u, -v,  1.0f)); // +Z
    case 5: return glm::normalize(glm::vec3(    -u, -v, -1.0f)); // -Z
    default: return glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

// 基于方向哈希的繁星：把方向量化到三维网格，每个 cell 按哈希生成一颗闪烁星点
glm::vec3 starField(const glm::vec3& dir) {
    const glm::vec3 q    = dir * 190.0f;
    const glm::vec3 cell = glm::floor(q);
    const glm::vec3 f    = q - cell;
    const float h = hash3(cell.x, cell.y, cell.z);

    glm::vec3 stars(0.0f);
    if (h > 0.9915f) {
        const glm::vec3 center(
            hash3(cell.x + 19.3f, cell.y +  7.1f, cell.z +  3.7f),
            hash3(cell.x +  5.7f, cell.y + 23.9f, cell.z + 11.3f),
            hash3(cell.x + 31.1f, cell.y +  2.3f, cell.z + 17.9f));
        const float d = glm::length(f - center);
        const float core = 1.0f - smoothstep(0.0f, 0.22f, d);
        // 静态 cubemap 用哈希亮度模拟闪烁，星点明暗错落
        const float twinkle = 0.35f + 0.65f *
            hash3(cell.x + 71.0f, cell.y + 13.0f, cell.z + 47.0f);
        stars = core * twinkle * glm::vec3(0.88f, 0.93f, 1.00f);
    }

    // 淡淡银河带：方向接近某条大圆时叠加细粒噪声
    const float band = std::exp(-34.0f * std::pow(
        glm::dot(dir, glm::normalize(glm::vec3(0.35f, 0.82f, -0.45f))), 2.0f));
    const float grain = hash3(std::floor(q.x * 0.5f),
                              std::floor(q.y * 0.5f),
                              std::floor(q.z * 0.5f));
    const float milky = band * (0.012f + 0.028f * grain);
    return stars + glm::vec3(milky * 0.70f, milky * 0.80f, milky);
}

// 长白山天池主峰 + 群山剪影的方位角天际线（dir.y 高度）
float mountainSkyline(float angle) {
    // 整数频率正弦保证 ±PI 接缝处连续
    float h = 0.045f
            + 0.020f * std::sin( 3.0f * angle + 0.7f)
            + 0.013f * std::sin( 7.0f * angle + 2.1f)
            + 0.007f * std::sin(13.0f * angle + 1.3f)
            + 0.006f * std::sin(23.0f * angle + 0.4f);

    // 主峰正对村口默认机位（相机朝 -Z，方位角约 -PI/2）
    const float d0 = angleDiff(angle, -1.70f);
    h += 0.145f * std::exp(-(d0 * d0) / (0.17f * 0.17f));
    const float d1 = angleDiff(angle, -2.30f);
    h += 0.060f * std::exp(-(d1 * d1) / (0.22f * 0.22f));
    const float d2 = angleDiff(angle, -1.05f);
    h += 0.050f * std::exp(-(d2 * d2) / (0.20f * 0.20f));
    return h;
}

// 单个方向上的完整天空着色
glm::vec3 shadeSky(const glm::vec3& dir, const SkyPalette& pal) {
    // 1) 天顶 -> 地平线的大气渐变 + 贴地暖/亮色带
    const float t = smoothstep(-0.05f, 0.62f, dir.y);
    glm::vec3 color = glm::mix(pal.horizon, pal.zenith, t);
    const float band = 1.0f - smoothstep(0.0f, 0.16f, std::fabs(dir.y - 0.01f));
    color = glm::mix(color, pal.horizonBand, band * 0.55f);
    const float below = 1.0f - smoothstep(-0.25f, 0.0f, dir.y);
    color = glm::mix(color, pal.ground, below * 0.85f);

    // 2) 太阳 / 月亮圆盘 + 柔和大气光晕
    const float cosA = glm::dot(dir, pal.celestialDir);
    const float c = std::max(cosA, 0.0f);
    const float innerHalo = std::pow(c, 220.0f);
    const float outerHalo = std::pow(c, 18.0f);
    color += pal.haloColor * (0.42f * innerHalo + 0.10f * outerHalo);
    if (cosA > 0.0f) {
        const float disk = smoothstep(0.99930f, 0.99975f, cosA);
        color = glm::mix(color, pal.orbColor, disk);
    }

    // 3) 星夜：方向哈希繁星 + 银河
    if (pal.stars && dir.y > 0.02f) {
        color += starField(dir) * smoothstep(0.02f, 0.16f, dir.y);
    }

    // 4) 地平线附近叠加长白山主峰与群山剪影，山巅带积雪高光
    if (dir.y < 0.26f) {
        const float angle = std::atan2(dir.z, dir.x);
        const float skyline = mountainSkyline(angle);
        const float inside = 1.0f - smoothstep(skyline - 0.012f, skyline + 0.002f, dir.y);
        if (inside > 0.0f) {
            const float depth = smoothstep(-0.22f, skyline, dir.y);
            glm::vec3 mountain = pal.mountain * (0.72f + 0.45f * depth);
            const float snowBand = 1.0f - smoothstep(0.0f, 0.030f, skyline - dir.y);
            const float peakWeight = smoothstep(0.090f, 0.180f, skyline);
            mountain = glm::mix(mountain, pal.snow, snowBand * peakWeight * 0.85f);
            color = glm::mix(color, mountain, inside);
        }
    }

    return glm::vec3(clamp01(color.r), clamp01(color.g), clamp01(color.b));
}

unsigned char toByte(float v) {
    return static_cast<unsigned char>(clamp01(v) * 255.0f + 0.5f);
}

} // namespace

unsigned int SkyGenerator::createProceduralCubemap(int presetType) {
    const int preset = std::clamp(presetType, 0, 2);
    const SkyPalette pal = makePalette(preset);
    std::vector<unsigned char> face(static_cast<size_t>(kFaceSize) * kFaceSize * 3);

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    for (int f = 0; f < 6; ++f) {
        for (int y = 0; y < kFaceSize; ++y) {
            const float v = 2.0f * (y + 0.5f) / kFaceSize - 1.0f;
            for (int x = 0; x < kFaceSize; ++x) {
                const float u = 2.0f * (x + 0.5f) / kFaceSize - 1.0f;
                const glm::vec3 dir = faceDirection(f, u, v);
                const glm::vec3 color = shadeSky(dir, pal);
                const size_t o = static_cast<size_t>(y * kFaceSize + x) * 3;
                face[o + 0] = toByte(color.r);
                face[o + 1] = toByte(color.g);
                face[o + 2] = toByte(color.b);
            }
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGB8,
                     kFaceSize, kFaceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, face.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return texture;
}
