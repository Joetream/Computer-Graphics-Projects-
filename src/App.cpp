#include "App.h"
#include "math/MathUtil.h"
#include "fx/SkyGenerator.h"
#include "fx/PlanarShadow.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_inverse.hpp>
#include "stb_image_write.h"
#include <iostream>
#include <filesystem>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace {
std::string assetPath(const std::string& rel) {
    return std::string(PROJECT_ROOT) + "/" + rel;
}
} // namespace

bool App::init(GLFWwindow* window, int width, int height) {
    m_window = window;
    m_width = width;
    m_height = height;

    m_input.init(window);

    if (MathUtil::verifyAgainstGLM()) {
        std::cout << "[MathUtil] Custom lookAt / perspective / ortho verified against GLM.\n";
    }

    // Load shaders
    m_uberShader.load(assetPath("shaders/uber_scene.vert"), assetPath("shaders/uber_scene.frag"));
    m_skyboxShader.load(assetPath("shaders/skybox.vert"), assetPath("shaders/skybox.frag"));
    m_particleShader.load(assetPath("shaders/particle.vert"), assetPath("shaders/particle.frag"));
    m_hudShader.load(assetPath("shaders/hud.vert"), assetPath("shaders/hud.frag"));

    // Load textures
    m_texSignGate.load2D(assetPath("assets/textures/sign_gate.png"), true, false);
    m_texSignGuide.load2D(assetPath("assets/textures/sign_guide.png"), true, false);
    m_texHelp.load2D(assetPath("assets/textures/help.png"), true, false);
    m_texRoofTile.load2D(assetPath("assets/textures/roof_tile.png"), true, false);

    // 2D Rasterization Sandbox dynamic texture
    m_texSandbox.createDynamicRGBA(Raster2DBoard::WIDTH, Raster2DBoard::HEIGHT, true);
    m_board2D.rebuild();
    m_texSandbox.updateSubRGBA(m_board2D.data());

    // Procedural cubemaps (Dawn, Noon, Night)
    for (int i = 0; i < 3; ++i) {
        m_skyCubemaps[i] = SkyGenerator::createProceduralCubemap(i);
    }
    m_skybox.init();
    m_hud.init();

    // Setup Clamped Cubic B-Spline camera tour path around the village
    std::vector<glm::vec3> tourPts = {
        {  0.0f,  4.2f,  42.0f},
        { 22.0f,  4.5f,  28.0f},
        { 30.0f,  5.5f,   4.0f},
        { 18.0f,  6.0f, -18.0f},
        { -8.0f,  5.2f, -22.0f},
        {-32.0f,  6.5f,  -8.0f},
        {-48.0f, 10.0f,  22.0f},
        {-18.0f, 14.0f,  52.0f},
        { 28.0f, 12.0f,  46.0f},
        {  0.0f,  4.2f,  42.0f}
    };
    m_tourPath.setControlPoints(tourPts, true);

    // Setup Korean houses in the inner ring R <= 35
    m_houseSpecs = {
        { {-10.0f, 0.0f, -10.0f},   0.0f, {1.15f, 1.15f, 1.15f}, true  }, // 主民居 (带独立火炕烟囱)
        { { 13.0f, 0.0f, -10.0f}, -18.0f, {1.00f, 1.00f, 1.00f}, true  }, // 东厢民居
        { {-16.0f, 0.0f,   8.0f},  35.0f, {0.92f, 0.95f, 0.92f}, false }, // 民俗工坊
        { { 16.0f, 0.0f,   9.0f}, -40.0f, {0.90f, 0.92f, 0.90f}, true  }  // 茶屋
    };
    rebuildVillageMeshes();

    m_terrain.init();
    m_water.init();
    m_riceField.init(m_terrain);
    m_hierarchy.init(m_terrain);
    m_particles.init(m_chimneyTops);

    // Default camera pose (Keyframe A: Dawn view of main gate & roofs)
    m_camera.setPose({0.0f, 4.2f, 42.0f}, -90.0f, -4.0f);

    return true;
}

void App::rebuildVillageMeshes() {
    KoreanVillageBuilder::buildAllHouses(m_houseSpecs, m_flags.bezierLevel, m_villageMesh, m_chimneyTops);
    KoreanVillageBuilder::buildWelcomeGate(m_flags.bezierLevel, m_gateMesh, m_plaqueQuad, m_plaqueModel);
    KoreanVillageBuilder::buildProps(m_propsMesh, m_sandboxTopQuad, m_sandboxTopModel, m_guideSignQuad, m_guideSignModel);
}

void App::onResize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

void App::handleHotkeys() {
    if (m_input.pressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
    if (m_input.pressed(GLFW_KEY_TAB)) {
        m_input.setCursorCaptured(!m_input.cursorCaptured());
    }
    if (m_input.pressed(GLFW_KEY_P)) {
        m_flags.touring = !m_flags.touring;
        m_camera.setTouring(m_flags.touring);
        std::cout << "[Mode] B-Spline Auto Tour: " << (m_flags.touring ? "ON" : "OFF") << "\n";
    }
    if (m_input.pressed(GLFW_KEY_G)) {
        m_flags.wireframe = !m_flags.wireframe;
        std::cout << "[Mode] Wireframe: " << (m_flags.wireframe ? "ON" : "OFF") << "\n";
    }
    if (m_input.pressed(GLFW_KEY_B)) {
        m_flags.bezierLevel = (m_flags.bezierLevel + 1) % 3;
        rebuildVillageMeshes();
        const int steps[3] = {4, 10, 20};
        std::cout << "[Bezier] Roof subdivision switched to "
                  << steps[m_flags.bezierLevel] << "x" << steps[m_flags.bezierLevel] << "\n";
    }
    if (m_input.pressed(GLFW_KEY_F)) {
        m_flags.shadeMode = (m_flags.shadeMode + 1) % 3;
        const char* names[3] = {"Flat (Per-Face)", "Gouraud (Per-Vertex)", "Blinn-Phong (Per-Fragment)"};
        std::cout << "[Shading] Mode: " << names[m_flags.shadeMode] << "\n";
    }
    if (m_input.pressed(GLFW_KEY_1)) {
        m_lighting.setPreset(0);
        std::cout << "[Lighting] Preset 1: Golden Autumn Dawn\n";
    }
    if (m_input.pressed(GLFW_KEY_2)) {
        m_lighting.setPreset(1);
        std::cout << "[Lighting] Preset 2: Bright Noon (Planar Shadows)\n";
    }
    if (m_input.pressed(GLFW_KEY_3)) {
        m_lighting.setPreset(2);
        std::cout << "[Lighting] Preset 3: Starry Night (6 Point Lights)\n";
    }
    if (m_input.pressed(GLFW_KEY_V)) {
        m_flags.ortho = !m_flags.ortho;
        m_camera.setOrtho(m_flags.ortho);
        std::cout << "[Projection] " << (m_flags.ortho ? "Orthographic Bird's-Eye" : "Perspective") << "\n";
    }
    if (m_input.pressed(GLFW_KEY_Z)) {
        m_flags.depthTest = !m_flags.depthTest;
        std::cout << "[Z-Buffer] GL_DEPTH_TEST: " << (m_flags.depthTest ? "ENABLED" : "DISABLED") << "\n";
    }
    if (m_input.pressed(GLFW_KEY_M)) {
        m_flags.showBoard = !m_flags.showBoard;
    }
    if (m_input.pressed(GLFW_KEY_K)) {
        m_flags.clipWindow = !m_flags.clipWindow;
        m_board2D.setClipEnabled(m_flags.clipWindow);
        std::cout << "[2D Clipping] Cohen-Sutherland Window: " << (m_flags.clipWindow ? "ON" : "OFF") << "\n";
    }
    if (m_flags.clipWindow) {
        int dx = (m_input.held(GLFW_KEY_RIGHT) ? 1 : 0) - (m_input.held(GLFW_KEY_LEFT) ? 1 : 0);
        int dy = (m_input.held(GLFW_KEY_UP) ? 1 : 0)    - (m_input.held(GLFW_KEY_DOWN) ? 1 : 0);
        if (dx != 0 || dy != 0) {
            m_board2D.moveClipWindow(dx, dy);
        }
    }
    if (m_input.pressed(GLFW_KEY_O)) {
        m_flags.season = 1 - m_flags.season;
        m_lighting.toggleSeason();
        m_particles.setMode(m_flags.season);
        std::cout << "[Season] Switched to: " << (m_flags.season == 0 ? "Golden Autumn (Smoke)" : "Early Winter (Snow)") << "\n";
    }
    if (m_input.pressed(GLFW_KEY_H)) {
        m_flags.showHelp = !m_flags.showHelp;
    }
    if (m_input.pressed(GLFW_KEY_R)) {
        m_uberShader.reload();
        m_skyboxShader.reload();
        m_particleShader.reload();
        m_hudShader.reload();
        std::cout << "[Shader] Hot-reloaded all GLSL shaders.\n";
    }
    if (m_input.pressed(GLFW_KEY_F12)) {
        saveScreenshot("screenshot.png");
    }
}

void App::update(float dt) {
    m_time += dt;
    m_input.update();
    handleHotkeys();

    // Pass 1: CPU-side state & physics updates
    if (m_flags.touring) {
        m_camera.updateTour(m_tourPath, dt);
    } else {
        m_camera.updateFreeRoam(m_input, dt);
    }

    m_lighting.update(dt);
    m_hierarchy.update(dt, m_lighting.sunDir());
    m_particles.update(dt, m_camera.position());

    if (m_board2D.consumeDirty()) {
        m_texSandbox.updateSubRGBA(m_board2D.data());
    }

    // Frustum culling for instanced rice paddies
    float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
    glm::mat4 vp = m_camera.projMatrix(aspect) * m_camera.viewMatrix();
    m_frustum.extract(vp);
    m_riceField.updateVisibleInstances(m_frustum);

    m_fpsTimer += dt;
    m_frameCount++;
    if (m_fpsTimer >= 2.0f) {
        float fps = m_frameCount / m_fpsTimer;
        std::string title = "延边朝鲜族民俗村·乡村振兴 | OpenGL 3.3 | FPS: " + std::to_string(static_cast<int>(fps))
                          + " | 稻田可见地块: " + std::to_string(m_riceField.visibleChunks())
                          + "/" + std::to_string(m_riceField.totalChunks()) + " (按 H 查看快捷键)";
        glfwSetWindowTitle(m_window, title.c_str());
        m_fpsTimer = 0.0f;
        m_frameCount = 0;
    }
}

void App::drawSkyboxPass(const glm::mat4& view, const glm::mat4& proj) {
    m_skyboxShader.use();
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
    m_skyboxShader.setMat4("u_View", viewNoTrans);
    m_skyboxShader.setMat4("u_Proj", proj);
    int skyA = m_lighting.skyIndexA();
    int skyB = m_lighting.skyIndexB();
    float blend = m_lighting.skyBlend();
    m_skybox.draw(m_skyboxShader, m_skyCubemaps[skyA], m_skyCubemaps[skyB], blend);
}

void App::render() {
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 proj = m_camera.projMatrix(aspect);

    // =========================================================================
    // Pass 2: Pre-Skybox Pass ONLY when GL_DEPTH_TEST is disabled (Key Z demo)
    // Prevents skybox from overwriting scene geometry when depth test is off!
    // =========================================================================
    if (!m_flags.depthTest) {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        drawSkyboxPass(view, proj);
        glDepthMask(GL_TRUE);
    }

    // =========================================================================
    // Pass 3: Main Opaque Geometry Pass
    // =========================================================================
    if (m_flags.depthTest) glEnable(GL_DEPTH_TEST);
    else                   glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glPolygonMode(GL_FRONT_AND_BACK, m_flags.wireframe ? GL_LINE : GL_FILL);

    m_uberShader.use();
    m_uberShader.setMat4("u_View", view);
    m_uberShader.setMat4("u_Proj", proj);
    m_uberShader.setVec3("u_CamPos", m_camera.position());
    m_uberShader.setInt("u_ShadeMode", m_flags.shadeMode);
    m_uberShader.setFloat("u_Time", m_time);
    m_uberShader.setBool("u_IsShadowPass", false);
    m_uberShader.setBool("u_Instanced", false);
    m_uberShader.setInt("u_AnimType", 0);
    m_uberShader.setBool("u_UseTexture", false);
    m_uberShader.setBool("u_UseEnvMap", false);
    m_uberShader.setFloat("u_Shininess", 36.0f);

    m_lighting.applyToShader(m_uberShader);

    // Bind skybox cubemaps to units 1 & 2 for water reflection
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyCubemaps[m_lighting.skyIndexA()]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyCubemaps[m_lighting.skyIndexB()]);
    m_uberShader.setInt("u_SkyTexA", 1);
    m_uberShader.setInt("u_SkyTexB", 2);
    m_uberShader.setFloat("u_SkyBlend", m_lighting.skyBlend());

    // 3.1 Terrain
    glm::mat4 identity(1.0f);
    m_uberShader.setMat4("u_Model", identity);
    m_uberShader.setMat3("u_NormalMat", glm::mat3(1.0f));
    m_terrain.draw(m_uberShader);

    // 3.2 Instanced Golden Rice Field (with vertex wind wave animation)
    m_uberShader.setBool("u_Instanced", true);
    m_uberShader.setInt("u_AnimType", 2);
    m_riceField.draw(m_uberShader);
    m_uberShader.setBool("u_Instanced", false);
    m_uberShader.setInt("u_AnimType", 0);

    // 3.3 Korean Traditional Village Houses, Gate & Props
    m_uberShader.setMat4("u_Model", identity);
    m_uberShader.setMat3("u_NormalMat", glm::mat3(1.0f));
    m_villageMesh.draw();
    m_gateMesh.draw();
    m_propsMesh.draw();
    m_lighting.drawLampPosts(m_uberShader);

    // 3.4 2D Sandbox table top (uses dynamic Bresenham/Scanline/Cohen-Sutherland texture)
    m_uberShader.setBool("u_UseTexture", true);
    m_texSandbox.bind(0);
    m_uberShader.setInt("u_AlbedoTex", 0);
    m_uberShader.setMat4("u_Model", m_sandboxTopModel);
    m_uberShader.setMat3("u_NormalMat", glm::inverseTranspose(glm::mat3(m_sandboxTopModel)));
    m_sandboxTopQuad.draw();

    // Guide Sign board
    m_texSignGuide.bind(0);
    m_uberShader.setMat4("u_Model", m_guideSignModel);
    m_uberShader.setMat3("u_NormalMat", glm::inverseTranspose(glm::mat3(m_guideSignModel)));
    m_guideSignQuad.draw();
    m_uberShader.setBool("u_UseTexture", false);

    // 3.5 Hierarchy animated objects (Windmills, Swing, Springboard, Solar Panels, Tour Cart)
    m_hierarchy.drawAll(m_uberShader, false);

    // 3.6 Burhatong River Bend Water (Sine wave vertex animation + Cubemap reflection)
    m_uberShader.setInt("u_AnimType", 1);
    m_uberShader.setBool("u_UseEnvMap", true);
    m_uberShader.setFloat("u_Shininess", 96.0f);
    m_uberShader.setMat4("u_Model", identity);
    m_uberShader.setMat3("u_NormalMat", glm::mat3(1.0f));
    m_water.draw(m_uberShader);
    m_uberShader.setInt("u_AnimType", 0);
    m_uberShader.setBool("u_UseEnvMap", false);

    // =========================================================================
    // Pass 4: Planar Projection Shadows (Daytime only) + Skybox (Normal Depth)
    // =========================================================================
    if (m_lighting.shadowEnabled() && m_flags.depthTest && !m_flags.wireframe) {
        glm::mat4 shadowMat = PlanarShadow::buildShadowMatrix(
            glm::vec4(0.0f, 1.0f, 0.0f, -0.015f),
            glm::vec4(m_lighting.sunDir(), 0.0f)
        );
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 0, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);

        m_uberShader.use();
        m_uberShader.setBool("u_IsShadowPass", true);
        m_uberShader.setMat4("u_ShadowProjMat", shadowMat);
        m_uberShader.setVec4("u_ShadowColor", glm::vec4(0.14f, 0.17f, 0.26f, 0.42f));

        m_uberShader.setMat4("u_Model", identity);
        m_villageMesh.draw();
        m_gateMesh.draw();
        m_hierarchy.drawAll(m_uberShader, true);

        m_uberShader.setBool("u_IsShadowPass", false);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (m_flags.depthTest) {
        glDepthFunc(GL_LEQUAL);
        drawSkyboxPass(view, proj);
        glDepthFunc(GL_LESS);
    }

    // =========================================================================
    // Pass 5: Alpha-Blended Bilingual Gate Plaque & Particle System
    // =========================================================================
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Bilingual Welcome Gate Plaque (sign_gate.png)
    m_uberShader.use();
    m_uberShader.setBool("u_UseTexture", true);
    m_texSignGate.bind(0);
    m_uberShader.setInt("u_AlbedoTex", 0);
    m_uberShader.setMat4("u_Model", m_plaqueModel);
    m_uberShader.setMat3("u_NormalMat", glm::inverseTranspose(glm::mat3(m_plaqueModel)));
    m_plaqueQuad.draw();
    m_uberShader.setBool("u_UseTexture", false);

    // Chimney Smoke / Winter Snow Particles
    m_particles.draw(m_particleShader, view, proj, m_camera.position());

    // =========================================================================
    // 2D HUD Overlays (2D Rasterization Sandbox Zoom [M] & Key Help [H])
    // =========================================================================
    glDisable(GL_DEPTH_TEST);
    if (m_flags.showBoard) {
        m_hud.drawQuad(m_hudShader, m_texSandbox, 0.36f, 0.22f, 0.96f, 0.94f, true);
    }
    if (m_flags.showHelp) {
        m_hud.drawQuad(m_hudShader, m_texHelp, -0.52f, -0.56f, 0.52f, 0.56f, false);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    if (m_flags.depthTest) glEnable(GL_DEPTH_TEST);
}

void App::saveScreenshot(const std::string& filename) const {
    std::vector<uint8_t> pixels(m_width * m_height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    stbi_flip_vertically_on_write(true);
    std::string outPath = assetPath(filename);
    if (stbi_write_png(outPath.c_str(), m_width, m_height, 3, pixels.data(), m_width * 3)) {
        std::cout << "[Screenshot] Saved to " << outPath << "\n";
    }
}

void App::captureComparisonSuite() {
    auto snap = [&](const std::string& relPath) {
        for (int i = 0; i < 4; ++i) {
            m_lighting.update(0.5f);
            m_hierarchy.update(0.16f, m_lighting.sunDir());
            m_particles.update(0.16f, m_camera.position());
            float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
            m_frustum.extract(m_camera.projMatrix(aspect) * m_camera.viewMatrix());
            m_riceField.updateVisibleInstances(m_frustum);
        }
        render();
        glFinish();
        saveScreenshot(relPath);
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    };

    // 1. Keyframe A: Dawn Hero View
    m_flags = RenderFlags{};
    m_lighting.setPreset(0);
    m_camera.setOrtho(false);
    m_camera.setPose({-2.0f, 3.8f, 44.0f}, -88.0f, -3.0f);
    snap("docs/images/render_dawn_hero.png");

    // 2. Keyframe B: Bright Noon + Planar Shadows + 2D Sandbox Overlay
    m_lighting.setPreset(1);
    m_camera.setPose({-6.0f, 5.2f, 16.0f}, -72.0f, -8.0f);
    m_flags.showBoard = true;
    m_flags.clipWindow = true;
    m_board2D.setClipEnabled(true);
    if (m_board2D.consumeDirty()) m_texSandbox.updateSubRGBA(m_board2D.data());
    snap("docs/images/render_noon_shadow_board.png");
    m_flags.showBoard = false;

    // 3. Keyframe C: Starry Night + 6 Point Lights
    m_lighting.setPreset(2);
    m_camera.setPose({0.0f, 3.6f, 42.0f}, -90.0f, -2.0f);
    snap("docs/images/render_night_lights.png");

    // 4. Wireframe Mode
    m_lighting.setPreset(1);
    m_camera.setPose({-8.0f, 6.0f, 12.0f}, -65.0f, -10.0f);
    m_flags.wireframe = true;
    snap("docs/images/render_wireframe.png");
    m_flags.wireframe = false;

    // 5. Orthographic Bird's-Eye Overview
    m_flags.ortho = true;
    m_camera.setOrtho(true);
    snap("docs/images/render_ortho_birdseye.png");
    m_flags.ortho = false;
    m_camera.setOrtho(false);
}
