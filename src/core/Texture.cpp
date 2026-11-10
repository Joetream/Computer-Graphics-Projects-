#include "core/Texture.h"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <iostream>
#include <utility>

Texture::~Texture() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_target(other.m_target), m_width(other.m_width), m_height(other.m_height) {
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_target = other.m_target;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_id = 0;
    }
    return *this;
}

bool Texture::load2D(const std::string& path, bool srgb, bool nearest) {
    stbi_set_flip_vertically_on_load(true);
    int w = 0, h = 0, ch = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!data) {
        std::cerr << "[Texture] Failed to load: " << path << "\n";
        return false;
    }

    if (m_id) glDeleteTextures(1, &m_id);
    glGenTextures(1, &m_id);
    m_target = GL_TEXTURE_2D;
    m_width = w;
    m_height = h;

    glBindTexture(GL_TEXTURE_2D, m_id);
    GLenum internalFmt = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
}

bool Texture::createDynamicRGBA(int width, int height, bool nearest) {
    if (m_id) glDeleteTextures(1, &m_id);
    glGenTextures(1, &m_id);
    m_target = GL_TEXTURE_2D;
    m_width = width;
    m_height = height;

    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Texture::updateSubRGBA(const uint8_t* rgbaPixels) {
    if (!m_id || !rgbaPixels) return;
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::adoptCubemap(unsigned int cubemapId) {
    if (m_id && m_id != cubemapId) glDeleteTextures(1, &m_id);
    m_id = cubemapId;
    m_target = GL_TEXTURE_CUBE_MAP;
}

void Texture::bind(unsigned int unit) const {
    if (!m_id) return;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(m_target, m_id);
}
