#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include "Settings.hpp"

class Camera;

struct RayStats {
    double avgIterations = 0.0;
    double capturedFraction = 0.0;
    double diskHitFraction = 0.0;
    bool valid = false;
};

class Renderer {
public:
    bool init(const std::string& shaderDir);
    void shutdown();
    bool reloadShaders();
    void render(const Camera& cam, const Settings& s, float diskTime,
                const glm::vec3& blackHolePosition, int winW, int winH);

    int internalW = 0, internalH = 0;
    RayStats stats;
    double gpuRayMs = 0.0;   // tempo do passe de ray tracing (timer query, assíncrono)
    double gpuTotalMs = 0.0; // tempo total de GPU do quadro

private:
    bool buildPrograms(GLuint& ray, GLuint& down, GLuint& blur, GLuint& tone);
    void createTargets(int w, int h);
    void destroyTargets();
    void pollAsyncResults();

    std::string shaderDir_;
    GLuint progRay_ = 0, progDown_ = 0, progBlur_ = 0, progTone_ = 0;
    GLuint texHDR_ = 0, texBloomA_ = 0, texBloomB_ = 0;
    int bloomW_ = 0, bloomH_ = 0;
    GLuint vao_ = 0;
    GLuint ssboStats_ = 0;
    GLsync statsFence_ = nullptr;
    unsigned frameCounter_ = 0;
    unsigned pendingPixels_ = 0;

    static constexpr int kQ = 4;
    GLuint qRay_[kQ] = {}, qTotalBegin_[kQ] = {}, qTotalEnd_[kQ] = {};
    bool qPending_[kQ] = {};
    int qIndex_ = 0;
    float tempNorm_ = 1.0f;
};
