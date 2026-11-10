#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

class Raster2DBoard {
public:
    static constexpr int WIDTH = 128;
    static constexpr int HEIGHT = 128;

    Raster2DBoard();
    void setClipEnabled(bool enabled);
    bool isClipEnabled() const { return m_clipEnabled; }
    void moveClipWindow(int dx, int dy);
    void rebuild();
    const uint8_t* data() const { return m_pixels.data(); }
    bool consumeDirty();

    void drawLineBresenham(int x0, int y0, int x1, int y1, glm::u8vec4 color);
    void drawCircleMidpoint(int cx, int cy, int radius, glm::u8vec4 color);
    void fillPolygonScanline(const std::vector<glm::ivec2>& poly, glm::u8vec4 color);
    bool clipLineCohenSutherland(float& x0, float& y0, float& x1, float& y1,
                                 float xmin, float ymin, float xmax, float ymax) const;

private:
    std::vector<uint8_t> m_pixels;
    bool m_clipEnabled = false;
    bool m_dirty = true;
    glm::ivec2 m_clipMin{28, 28};
    glm::ivec2 m_clipMax{100, 96};

    void setPixel(int x, int y, glm::u8vec4 color);
};
