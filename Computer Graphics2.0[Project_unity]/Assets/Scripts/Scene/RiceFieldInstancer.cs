// ============================================================================
// WP-U4 · Assets/Scripts/Scene/RiceFieldInstancer.cs
// 万株金秋稻浪 GPU Instancing 渲染器 + Gribb-Hartmann 视锥剔除统计：
//   · 将稻田划分为 16 个空间分块（Chunk AABB），每帧调用 FrustumCuller.TestAABB
//   · 对视锥内的稻穗实例使用 Graphics.DrawMeshInstanced 批量提交（配合顶点风浪动画）
//   · 实时向答辩 HUD 报告 VisibleInstances / TotalInstances 与视锥剔除率
// ============================================================================
using System.Collections.Generic;
using UnityEngine;
using YanbianCG.CGCore;

namespace YanbianCG.Scene
{
    public class RiceFieldInstancer : MonoBehaviour
    {
        private class RiceChunk
        {
            public Vector3 boundsMin;
            public Vector3 boundsMax;
            public readonly List<Matrix4x4> matrices = new List<Matrix4x4>();
        }

        private Mesh _riceClusterMesh;
        private Material _riceMaterial;
        private readonly FrustumCuller _frustum = new FrustumCuller();
        private readonly List<RiceChunk> _chunks = new List<RiceChunk>();
        private readonly List<Matrix4x4[]> _batchBuffers = new List<Matrix4x4[]>();

        public bool EnableFrustumCulling { get; set; } = true;
        public int TotalInstances { get; private set; }
        public int VisibleInstances { get; private set; }
        public int VisibleChunks { get; private set; }
        public int TotalChunks => _chunks.Count;

        public void Initialize(Material baseUberShaderMat)
        {
            _riceClusterMesh = ProceduralMeshLib.BuildCrossRiceClusterMesh();
            _riceMaterial = new Material(baseUberShaderMat)
            {
                name = "Mat_InstancedGoldenRice",
                enableInstancing = true
            };
            _riceMaterial.SetFloat("_WindAnim", 1.0f);
            _riceMaterial.SetFloat("_SpecStrength", 0.08f);

            GenerateRiceFieldChunks();
        }

        private void GenerateRiceFieldChunks()
        {
            _chunks.Clear();
            TotalInstances = 0;

            // 村落东西两侧与南侧金秋稻田区域，按 8×4 空间区块组织 AABB
            System.Random rng = new System.Random(2026);
            float[] regionMinX = { -48f,  16f };
            float[] regionMaxX = { -16f,  48f };

            for (int side = 0; side < 2; ++side)
            {
                for (int cx = 0; cx < 4; ++cx)
                {
                    for (int cz = 0; cz < 4; ++cz)
                    {
                        float x0 = Mathf.Lerp(regionMinX[side], regionMaxX[side], cx / 4f);
                        float x1 = Mathf.Lerp(regionMinX[side], regionMaxX[side], (cx + 1) / 4f);
                        float z0 = Mathf.Lerp(-20f, 22f, cz / 4f);
                        float z1 = Mathf.Lerp(-20f, 22f, (cz + 1) / 4f);

                        RiceChunk chunk = new RiceChunk
                        {
                            boundsMin = new Vector3(x0 - 0.5f, -1f, z0 - 0.5f),
                            boundsMax = new Vector3(x1 + 0.5f,  3f, z1 + 0.5f)
                        };

                        // 每个分块生成 100 株金黄稻穗（共 32 个分块 = 3,200 株实例，约 19,200 个三角面）
                        for (int k = 0; k < 100; ++k)
                        {
                            float rx = Mathf.Lerp(x0, x1, (float)rng.NextDouble());
                            float rz = Mathf.Lerp(z0, z1, (float)rng.NextDouble());
                            float ry = ProceduralMeshLib.SampleTerrainHeight(rx, rz);
                            float yaw = (float)rng.NextDouble() * 360f;
                            float sc  = 0.85f + (float)rng.NextDouble() * 0.45f;

                            Matrix4x4 m = Matrix4x4.TRS(
                                new Vector3(rx, ry, rz),
                                Quaternion.Euler(0f, yaw, 0f),
                                new Vector3(sc, sc * 1.1f, sc)
                            );
                            chunk.matrices.Add(m);
                            TotalInstances++;
                        }

                        _chunks.Add(chunk);
                    }
                }
            }

            // 预分配每批最多 1023 个实例的缓冲池
            for (int i = 0; i < 6; ++i)
                _batchBuffers.Add(new Matrix4x4[1023]);
        }

        private void Update()
        {
            if (_riceClusterMesh == null || _riceMaterial == null) return;
            Camera cam = Camera.main;
            if (cam == null) return;

            // 使用手写 Gribb-Hartmann 视锥平面提取器
            Matrix4x4 vp = cam.projectionMatrix * cam.worldToCameraMatrix;
            _frustum.ExtractFromMatrix(vp);

            VisibleInstances = 0;
            VisibleChunks = 0;

            int batchIdx = 0;
            int countInBatch = 0;

            foreach (var chunk in _chunks)
            {
                if (EnableFrustumCulling && !_frustum.TestAABB(chunk.boundsMin, chunk.boundsMax))
                    continue;

                VisibleChunks++;
                var list = chunk.matrices;
                for (int i = 0; i < list.Count; ++i)
                {
                    _batchBuffers[batchIdx][countInBatch++] = list[i];
                    VisibleInstances++;

                    if (countInBatch == 1023)
                    {
                        Graphics.DrawMeshInstanced(_riceClusterMesh, 0, _riceMaterial, _batchBuffers[batchIdx], countInBatch);
                        batchIdx = Mathf.Min(batchIdx + 1, _batchBuffers.Count - 1);
                        countInBatch = 0;
                    }
                }
            }

            if (countInBatch > 0)
            {
                Graphics.DrawMeshInstanced(_riceClusterMesh, 0, _riceMaterial, _batchBuffers[batchIdx], countInBatch);
            }
        }
    }
}
