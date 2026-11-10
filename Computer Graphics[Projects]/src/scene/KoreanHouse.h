#pragma once
#include "core/Mesh.h"
#include <glm/glm.hpp>
#include <vector>

struct HouseInstance {
    glm::vec3 pos;
    float yawDeg;
    glm::vec3 scale;
    bool hasChimney;
};

class KoreanVillageBuilder {
public:
    static void buildAllHouses(const std::vector<HouseInstance>& houses, int bezierLevel,
                               Mesh& outVillageMesh, std::vector<glm::vec3>& outChimneyTops);
    static void buildWelcomeGate(int bezierLevel, Mesh& outGateMesh, Mesh& outPlaqueQuad, glm::mat4& outPlaqueModel);
    static void buildProps(Mesh& outPropsMesh, Mesh& outSandboxTopQuad, glm::mat4& outSandboxTopModel,
                           Mesh& outGuideSignQuad, glm::mat4& outGuideSignModel);
};
