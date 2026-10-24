#include "ShaderUtil.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace shaderutil {

bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::fprintf(stderr, "[shader] nao foi possivel abrir %s\n", path.c_str()); return false; }
    std::stringstream ss; ss << f.rdbuf(); out = ss.str();
    return true;
}

static GLuint compileStage(GLenum type, const std::string& path) {
    std::string src;
    if (!readFile(path, src)) return 0;
    GLuint s = glCreateShader(type);
    const char* p = src.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(size_t(std::max(len, 1)));
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "[shader] erro ao compilar %s:\n%s\n", path.c_str(), log.data());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint linkProgram(const GLuint* shaders, int count, const std::string& name) {
    GLuint prog = glCreateProgram();
    for (int i = 0; i < count; ++i) glAttachShader(prog, shaders[i]);
    glLinkProgram(prog);
    for (int i = 0; i < count; ++i) { glDetachShader(prog, shaders[i]); glDeleteShader(shaders[i]); }
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(size_t(std::max(len, 1)));
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::fprintf(stderr, "[shader] erro de link em %s:\n%s\n", name.c_str(), log.data());
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

GLuint compileComputeProgram(const std::string& path) {
    GLuint cs = compileStage(GL_COMPUTE_SHADER, path);
    if (!cs) return 0;
    return linkProgram(&cs, 1, path);
}

GLuint compileGraphicsProgram(const std::string& vp, const std::string& fp) {
    GLuint vs = compileStage(GL_VERTEX_SHADER, vp);
    if (!vs) return 0;
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fp);
    if (!fs) { glDeleteShader(vs); return 0; }
    GLuint s[2] = { vs, fs };
    return linkProgram(s, 2, fp);
}

} // namespace shaderutil
