#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool load(const std::string& vertPath, const std::string& fragPath);
    bool reload();
    void use() const;
    unsigned int id() const { return m_program; }

    void setInt(const std::string& name, int v) const;
    void setBool(const std::string& name, bool v) const;
    void setFloat(const std::string& name, float v) const;
    void setVec2(const std::string& name, const glm::vec2& v) const;
    void setVec3(const std::string& name, const glm::vec3& v) const;
    void setVec4(const std::string& name, const glm::vec4& v) const;
    void setMat3(const std::string& name, const glm::mat3& m) const;
    void setMat4(const std::string& name, const glm::mat4& m) const;

private:
    unsigned int m_program = 0;
    std::string m_vertPath;
    std::string m_fragPath;
    mutable std::unordered_map<std::string, int> m_uniformCache;

    int getLocation(const std::string& name) const;
    static std::string readFileWithIncludes(const std::string& path, int depth = 0);
    static unsigned int compileStage(unsigned int type, const std::string& src, const std::string& debugName);
};
