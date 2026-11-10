#pragma once
#include <string>
#include <cstdint>

class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load2D(const std::string& path, bool srgb = true, bool nearest = false);
    bool createDynamicRGBA(int width, int height, bool nearest = true);
    void updateSubRGBA(const uint8_t* rgbaPixels);
    void adoptCubemap(unsigned int cubemapId);

    void bind(unsigned int unit = 0) const;
    unsigned int id() const { return m_id; }
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    unsigned int m_id = 0;
    unsigned int m_target = 0; // GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP
    int m_width = 0;
    int m_height = 0;
};
