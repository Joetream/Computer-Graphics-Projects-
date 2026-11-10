#pragma once
#include "core/Shader.h"
#include "core/Mesh.h"
#include "core/Texture.h"
#include "core/Camera.h"
#include "core/Input.h"
#include "math/Frustum.h"
#include "math/BSplinePath.h"
#include "scene/KoreanHouse.h"
#include "scene/HierarchyObjects.h"
#include "scene/Terrain.h"
#include "scene/Water.h"
#include "scene/RiceField.h"
#include "fx/LightingState.h"
#include "fx/Skybox.h"
#include "fx/ParticleSystem.h"
#include "ui/Raster2DBoard.h"
#include "ui/Hud.h"
#include <vector>
#include <string>

struct GLFWwindow;

struct RenderFlags {
    int  shadeMode   = 2;     // 0 Flat, 1 Gouraud, 2 Phong      [F]
    int  bezierLevel = 1;     // 0:4x4, 1:10x10, 2:20x20        [B]
    bool wireframe   = false; //                                [G]
    bool depthTest   = true;  //                                [Z]
    bool ortho       = false; //                                [V]
    bool touring     = false; // B 样条巡游                      [P]
    bool showBoard   = false; //                                [M]
    bool clipWindow  = false; //                                [K]
    bool showHelp    = false; //                                [H]
    int  season      = 0;     // 0 金秋, 1 初冬                  [O]
};

class App {
public:
    bool init(GLFWwindow* window, int width, int height);
    void onResize(int width, int height);
    void update(float dt);
    void render();
    void saveScreenshot(const std::string& filename) const;

private:
    GLFWwindow* m_window = nullptr;
    int m_width = 1600;
    int m_height = 900;
    float m_time = 0.0f;
    float m_fpsTimer = 0.0f;
    int m_frameCount = 0;

    RenderFlags m_flags;
    Input m_input;
    Camera m_camera;
    Frustum m_frustum;
    BSplinePath m_tourPath;

    // Shaders
    Shader m_uberShader;
    Shader m_skyboxShader;
    Shader m_particleShader;
    Shader m_hudShader;

    // Scene geometry
    std::vector<HouseInstance> m_houseSpecs;
    std::vector<glm::vec3> m_chimneyTops;
    Mesh m_villageMesh;
    Mesh m_gateMesh;
    Mesh m_plaqueQuad;
    glm::mat4 m_plaqueModel{1.0f};
    Mesh m_propsMesh;
    Mesh m_sandboxTopQuad;
    glm::mat4 m_sandboxTopModel{1.0f};
    Mesh m_guideSignQuad;
    glm::mat4 m_guideSignModel{1.0f};

    HierarchyObjects m_hierarchy;
    Terrain m_terrain;
    Water m_water;
    RiceField m_riceField;

    // Effects & Sky
    LightingState m_lighting;
    Skybox m_skybox;
    unsigned int m_skyCubemaps[3] = {0, 0, 0};
    ParticleSystem m_particles;

    // Textures & 2D UI
    Texture m_texSignGate;
    Texture m_texSignGuide;
    Texture m_texHelp;
    Texture m_texRoofTile;
    Texture m_texSandbox;
    Raster2DBoard m_board2D;
    Hud m_hud;

    void rebuildVillageMeshes();
    void handleHotkeys();
    void drawSkyboxPass(const glm::mat4& view, const glm::mat4& proj);
};
