#pragma once
#include "core/Mesh.h"
#include "core/Shader.h"
#include "math/BSplinePath.h"
#include <glm/glm.hpp>

class Terrain;

class HierarchyObjects {
public:
    void init(const Terrain& terrain);
    void update(float dt, const glm::vec3& sunDir);
    void drawAll(const Shader& shader, bool shadowPassOnly = false) const;

private:
    Mesh m_towerMesh;
    Mesh m_nacelleMesh;
    Mesh m_hubMesh;
    Mesh m_bladeMesh;

    Mesh m_swingFrameMesh;
    Mesh m_swingSeatMesh;

    Mesh m_seesawBaseMesh;
    Mesh m_seesawPlankMesh;

    Mesh m_solarPoleMesh;
    Mesh m_solarPanelMesh;

    Mesh m_cartBodyMesh;
    Mesh m_cartWheelMesh;

    float m_rotorAngle = 0.0f;
    float m_nacelleYaw = 0.0f;
    float m_swingTime = 0.0f;
    float m_solarPitch = 35.0f;
    float m_cartDist = 15.0f;
    float m_wheelRoll = 0.0f;
    float m_cartPitch = 0.0f;
    const Terrain* m_terrain = nullptr;
    BSplinePath m_cartPath;
    glm::vec3 m_cartPos{0.0f, 0.0f, 32.0f};
    glm::vec3 m_cartDir{1.0f, 0.0f, 0.0f};
};
