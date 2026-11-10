#include "fx/PlanarShadow.h"

glm::mat4 PlanarShadow::buildShadowMatrix(const glm::vec4& groundPlane,
                                          const glm::vec4& lightDir) {
    // =====================================================================
    // 平面投影阴影矩阵推导（列向量约定 P * V * M * v）
    //
    // 地面平面： n · X + d = 0，齐次平面向量 plane = (nx, ny, nz, d)
    // 平行光向： L = (Lx, Ly, Lz, 0)，w = 0 表示方向而非位置
    //
    // 空间点 P 沿光方向投射到平面上的交点：
    //     P' = P - L * (plane · P) / (plane · L)
    // 齐次化（乘以 plane · L）后可写成线性变换：
    //     P' ∝ [ (plane · L) I - L * plane^T ] * P
    // 其中 I 为 4x4 单位阵，L * plane^T 为 4 维列向量与行向量的外积。
    //
    // 因此按 GLM 列主序下标 M[col][row]（即题目中的 M_{c,r}）填充：
    //     M[c][r] = (plane · L) * delta(r, c) - L[r] * plane[c]
    // 当 |plane · L| 接近 0（光线与地面平行）时矩阵退化，调用方应跳过阴影 Pass。
    // =====================================================================
    const float dotPlaneLight = glm::dot(groundPlane, lightDir);

    glm::mat4 shadow(0.0f);
    for (int c = 0; c < 4; ++c) {          // 列
        for (int r = 0; r < 4; ++r) {      // 行
            const float delta = (r == c) ? 1.0f : 0.0f;
            shadow[c][r] = delta * dotPlaneLight - lightDir[r] * groundPlane[c];
        }
    }
    return shadow;
}
