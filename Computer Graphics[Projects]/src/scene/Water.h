#pragma once
#include "core/Mesh.h"
#include "core/Shader.h"

class Water {
public:
    void init();
    void draw(const Shader& shader) const;

private:
    Mesh m_mesh;
};
