#pragma once
#include "core/Shader.h"
#include <vector>
#include <glm/glm.hpp>

class ParticleSystem {
public:
    ParticleSystem() = default;
    ~ParticleSystem();

    void init(const std::vector<glm::vec3>& chimneyTops);
    void setMode(int mode); // 0 = Chimney Smoke (Autumn), 1 = Snowfall (Winter)
    void update(float dt, const glm::vec3& camPos);
    void draw(const Shader& particleShader, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos) const;

private:
    struct Particle {
        glm::vec3 pos;
        glm::vec3 vel;
        float life;
        float maxLife;
        float size;
        float alpha;
    };
    struct GPUInstance {
        glm::vec3 pos;
        float size;
        glm::vec4 color;
    };

    int m_mode = 0;
    std::vector<glm::vec3> m_emitters;
    std::vector<Particle> m_particles;
    mutable std::vector<GPUInstance> m_gpuBuffer;

    unsigned int m_vao = 0;
    unsigned int m_quadVbo = 0;
    unsigned int m_instVbo = 0;

    void respawnParticle(Particle& p, int idx, const glm::vec3& camPos);
};
