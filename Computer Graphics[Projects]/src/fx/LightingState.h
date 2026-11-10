#pragma once
#include "core/Mesh.h"
#include "core/Shader.h"
#include <glm/glm.hpp>

class LightingState {
public:
    LightingState();

    void setPreset(int index); // 0: Dawn, 1: Noon, 2: Night
    void toggleSeason();       // 0: Autumn, 1: Winter
    void update(float dt);
    void applyToShader(const Shader& shader) const;
    void drawLampPosts(const Shader& shader) const;

    glm::vec3 sunDir() const { return m_currSunDir; }
    bool shadowEnabled() const { return m_targetPreset != 2; }
    int skyIndexA() const { return m_skyA; }
    int skyIndexB() const { return m_skyB; }
    float skyBlend() const { return m_skyBlend; }
    float snowAmount() const { return m_snowAmount; }

private:
    struct Preset {
        glm::vec3 sunDir;
        glm::vec3 sunColor;
        glm::vec3 ambient;
        glm::vec3 fogColor;
        float fogDensity;
        float pointLightFactor;
    };
    Preset m_presets[3];
    int m_prevPreset = 0;
    int m_targetPreset = 0;
    float m_transition = 1.0f;

    glm::vec3 m_currSunDir{0.6f, 0.28f, -0.5f};
    glm::vec3 m_currSunColor{1.0f, 0.72f, 0.45f};
    glm::vec3 m_currAmbient{0.25f, 0.22f, 0.26f};
    glm::vec3 m_currFogColor{0.85f, 0.65f, 0.52f};
    float m_currFogDensity = 0.0055f;
    float m_currPointFactor = 0.0f;

    int m_skyA = 0;
    int m_skyB = 0;
    float m_skyBlend = 0.0f;

    int m_seasonTarget = 0;
    float m_snowAmount = 0.0f;

    glm::vec3 m_lampPos[6];
    glm::vec3 m_lampColor[6];
    mutable Mesh m_lampPostMesh;
    mutable bool m_lampMeshBuilt = false;
};
