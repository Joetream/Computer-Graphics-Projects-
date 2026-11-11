#pragma once
#include "core/Mesh.h"
#include "core/Shader.h"

class Terrain {
public:
    void init();
    void draw(const Shader& shader) const;
    float getHeight(float x, float z) const;

private:
    Mesh m_mesh;
};
