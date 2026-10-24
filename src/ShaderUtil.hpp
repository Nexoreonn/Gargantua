#pragma once
#include <GL/glew.h>
#include <string>

namespace shaderutil {
    bool readFile(const std::string& path, std::string& out);
    // Retornam 0 em caso de erro (log impresso no stderr).
    GLuint compileComputeProgram(const std::string& path);
    GLuint compileGraphicsProgram(const std::string& vertPath, const std::string& fragPath);
}
