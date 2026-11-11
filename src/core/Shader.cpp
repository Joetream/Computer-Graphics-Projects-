#include "core/Shader.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

Shader::~Shader() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

Shader::Shader(Shader&& other) noexcept
    : m_program(other.m_program),
      m_vertPath(std::move(other.m_vertPath)),
      m_fragPath(std::move(other.m_fragPath)),
      m_uniformCache(std::move(other.m_uniformCache)) {
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_program != 0) glDeleteProgram(m_program);
        m_program = other.m_program;
        m_vertPath = std::move(other.m_vertPath);
        m_fragPath = std::move(other.m_fragPath);
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_program = 0;
    }
    return *this;
}

std::string Shader::readFileWithIncludes(const std::string& path, int depth) {
    if (depth > 8) return "";
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "[Shader] Failed to open file: " << path << "\n";
        return "";
    }
    std::filesystem::path parentDir = std::filesystem::path(path).parent_path();
    std::stringstream out;
    std::string line;
    while (std::getline(ifs, line)) {
        const std::string tag = "#include";
        auto pos = line.find(tag);
        if (pos != std::string::npos) {
            auto q1 = line.find('"', pos);
            auto q2 = line.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                std::string rel = line.substr(q1 + 1, q2 - q1 - 1);
                std::filesystem::path incPath = parentDir / rel;
                out << readFileWithIncludes(incPath.string(), depth + 1) << "\n";
                continue;
            }
        }
        out << line << "\n";
    }
    return out.str();
}

unsigned int Shader::compileStage(unsigned int type, const std::string& src, const std::string& debugName) {
    unsigned int shader = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Compile error in " << debugName << ":\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::load(const std::string& vertPath, const std::string& fragPath) {
    m_vertPath = vertPath;
    m_fragPath = fragPath;

    std::string vsrc = readFileWithIncludes(vertPath);
    std::string fsrc = readFileWithIncludes(fragPath);
    if (vsrc.empty() || fsrc.empty()) return false;

    unsigned int vs = compileStage(GL_VERTEX_SHADER, vsrc, vertPath);
    unsigned int fs = compileStage(GL_FRAGMENT_SHADER, fsrc, fragPath);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Link error (" << vertPath << " + " << fragPath << "):\n" << log << "\n";
        glDeleteProgram(prog);
        return false;
    }

    if (m_program != 0) glDeleteProgram(m_program);
    m_program = prog;
    m_uniformCache.clear();
    return true;
}

bool Shader::reload() {
    if (m_vertPath.empty() || m_fragPath.empty()) return false;
    return load(m_vertPath, m_fragPath);
}

void Shader::use() const {
    glUseProgram(m_program);
}

int Shader::getLocation(const std::string& name) const {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) return it->second;
    int loc = glGetUniformLocation(m_program, name.c_str());
    m_uniformCache[name] = loc;
    return loc;
}

void Shader::setInt(const std::string& name, int v) const {
    int loc = getLocation(name);
    if (loc >= 0) glUniform1i(loc, v);
}

void Shader::setBool(const std::string& name, bool v) const {
    setInt(name, v ? 1 : 0);
}

void Shader::setFloat(const std::string& name, float v) const {
    int loc = getLocation(name);
    if (loc >= 0) glUniform1f(loc, v);
}

void Shader::setVec2(const std::string& name, const glm::vec2& v) const {
    int loc = getLocation(name);
    if (loc >= 0) glUniform2fv(loc, 1, glm::value_ptr(v));
}

void Shader::setVec3(const std::string& name, const glm::vec3& v) const {
    int loc = getLocation(name);
    if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(v));
}

void Shader::setVec4(const std::string& name, const glm::vec4& v) const {
    int loc = getLocation(name);
    if (loc >= 0) glUniform4fv(loc, 1, glm::value_ptr(v));
}

void Shader::setMat3(const std::string& name, const glm::mat3& m) const {
    int loc = getLocation(name);
    if (loc >= 0) glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::setMat4(const std::string& name, const glm::mat4& m) const {
    int loc = getLocation(name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}
