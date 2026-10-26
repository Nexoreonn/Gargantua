#include "Renderer.hpp"
#include "Camera.hpp"
#include "Physics.hpp"
#include "ShaderUtil.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

static void createTex(GLuint& tex, int w, int h) {
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, w, h);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool Renderer::init(const std::string& shaderDir) {
    shaderDir_ = shaderDir;
    if (!buildPrograms(progRay_, progDown_, progBlur_, progTone_, progTaa_)) return false;

    glGenVertexArrays(1, &vao_); // VAO vazio: triângulo de tela cheia gerado no vertex shader

    glGenBuffers(1, &ssboStats_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboStats_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(GLuint), nullptr, GL_DYNAMIC_READ);

    glGenQueries(kQ, qRay_);
    glGenQueries(kQ, qTotalBegin_);
    glGenQueries(kQ, qTotalEnd_);

    tempNorm_ = physics::temperatureNormalization();
    return true;
}

bool Renderer::buildPrograms(GLuint& ray, GLuint& down, GLuint& blur, GLuint& tone, GLuint& taa) {
    ray  = shaderutil::compileComputeProgram(shaderDir_ + "/raytrace.comp");
    down = shaderutil::compileComputeProgram(shaderDir_ + "/bloom_down.comp");
    blur = shaderutil::compileComputeProgram(shaderDir_ + "/bloom_blur.comp");
    tone = shaderutil::compileGraphicsProgram(shaderDir_ + "/fullscreen.vert",
                                              shaderDir_ + "/tonemap.frag");
    taa = shaderutil::compileComputeProgram(shaderDir_ + "/taa.comp");
    if (ray && down && blur && tone && taa) return true;
    if (ray) glDeleteProgram(ray);
    if (down) glDeleteProgram(down);
    if (blur) glDeleteProgram(blur);
    if (tone) glDeleteProgram(tone);
    if (taa) glDeleteProgram(taa);
    ray = down = blur = tone = taa = 0;
    return false;
}

bool Renderer::reloadShaders() {
    GLuint r, d, b, t, taa;
    if (!buildPrograms(r, d, b, t, taa)) {
        std::fprintf(stderr, "[renderer] recarga falhou; mantendo shaders anteriores\n");
        return false;
    }
    glDeleteProgram(progRay_); glDeleteProgram(progDown_);
    glDeleteProgram(progBlur_); glDeleteProgram(progTone_);
    glDeleteProgram(progTaa_);
    progRay_ = r; progDown_ = d; progBlur_ = b; progTone_ = t; progTaa_ = taa;
    std::printf("[renderer] shaders recarregados\n");
    return true;
}

void Renderer::createTargets(int w, int h) {
    destroyTargets();
    internalW = w; internalH = h;
    bloomW_ = std::max(1, w / 4);
    bloomH_ = std::max(1, h / 4);
    createTex(texHDR_, w, h);
    createTex(texBloomA_, bloomW_, bloomH_);
    createTex(texBloomB_, bloomW_, bloomH_);
    createTex(texTaa_[0], w, h);
    createTex(texTaa_[1], w, h);
    resetTemporalHistory();
}

void Renderer::destroyTargets() {
    GLuint t[5] = { texHDR_, texBloomA_, texBloomB_, texTaa_[0], texTaa_[1] };
    glDeleteTextures(5, t);
    texHDR_ = texBloomA_ = texBloomB_ = 0;
    texTaa_[0] = texTaa_[1] = 0;
    resetTemporalHistory();
}

void Renderer::resetTemporalHistory() {
    taaReadIndex_ = 0;
    taaHistoryValid_ = false;
    previousCameraValid_ = false;
}

void Renderer::shutdown() {
    destroyTargets();
    if (statsFence_) { glDeleteSync(statsFence_); statsFence_ = nullptr; }
    glDeleteBuffers(1, &ssboStats_);
    glDeleteQueries(kQ, qRay_);
    glDeleteQueries(kQ, qTotalBegin_);
    glDeleteQueries(kQ, qTotalEnd_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(progRay_); glDeleteProgram(progDown_);
    glDeleteProgram(progBlur_); glDeleteProgram(progTone_);
    glDeleteProgram(progTaa_);
}

// Leituras assíncronas: só lê da GPU quando o resultado já está pronto (sem stall).
void Renderer::pollAsyncResults() {
    if (statsFence_) {
        GLenum st = glClientWaitSync(statsFence_, 0, 0);
        if (st == GL_ALREADY_SIGNALED || st == GL_CONDITION_SATISFIED) {
            GLuint data[4] = {};
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboStats_);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
            const double px = std::max(1u, data[2]);
            stats.avgIterations = data[0] / px;
            stats.capturedFraction = data[1] / px;
            stats.diskHitFraction = data[3] / px;
            stats.valid = true;
            glDeleteSync(statsFence_);
            statsFence_ = nullptr;
        }
    }
    for (int i = 0; i < kQ; ++i) {
        if (!qPending_[i]) continue;
        GLint avail = 0;
        glGetQueryObjectiv(qTotalEnd_[i], GL_QUERY_RESULT_AVAILABLE, &avail);
        if (!avail) continue;
        GLuint64 ray = 0, t0 = 0, t1 = 0;
        glGetQueryObjectui64v(qRay_[i], GL_QUERY_RESULT, &ray);
        glGetQueryObjectui64v(qTotalBegin_[i], GL_QUERY_RESULT, &t0);
        glGetQueryObjectui64v(qTotalEnd_[i], GL_QUERY_RESULT, &t1);
        gpuRayMs   = 0.9 * gpuRayMs   + 0.1 * (ray * 1e-6);
        gpuTotalMs = 0.9 * gpuTotalMs + 0.1 * ((t1 - t0) * 1e-6);
        qPending_[i] = false;
    }
}

void Renderer::render(const Camera& cam, const Settings& s, float diskTime,
                      const glm::vec3& blackHolePosition, int winW, int winH) {
    if (winW <= 0 || winH <= 0) return;
    pollAsyncResults();

    int sampleScaleX = 1;
    int sampleScaleY = 1;
    switch (s.antiAliasing) {
    case AntiAliasingMode::MSAA2x: sampleScaleX = 2; break;
    case AntiAliasingMode::MSAA4x: sampleScaleX = 2; sampleScaleY = 2; break;
    case AntiAliasingMode::MSAA8x: sampleScaleX = 4; sampleScaleY = 2; break;
    default: break;
    }
    const int iw = std::max(16, int(std::lround(winW * s.renderScale * sampleScaleX)));
    const int ih = std::max(16, int(std::lround(winH * s.renderScale * sampleScaleY)));
    if (iw != internalW || ih != internalH || !texHDR_) createTargets(iw, ih);

    const int q = qIndex_;
    const bool queryFree = !qPending_[q];
    if (queryFree) glQueryCounter(qTotalBegin_[q], GL_TIMESTAMP);

    // ---------- Passe 1: traçado de geodésicas ----------
    const QualityPreset& qp = s.preset();
    const bool collect = (statsFence_ == nullptr) && (frameCounter_ % 15 == 0);
    if (collect) {
        const GLuint zero[4] = { 0, 0, 0, 0 };
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboStats_);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zero), zero);
    }

    glUseProgram(progRay_);
    const glm::vec3 p = cam.position - blackHolePosition;
    const glm::vec3 f = cam.forward(), r = cam.right(), u = cam.up();
    const float tanHalf = std::tan(glm::radians(cam.fovDeg) * 0.5f);
    auto L = [&](const char* n) { return glGetUniformLocation(progRay_, n); };
    glUniform3f(L("uCamPos"), p.x, p.y, p.z);
    glUniform3f(L("uCamFwd"), f.x, f.y, f.z);
    glUniform3f(L("uCamRight"), r.x, r.y, r.z);
    glUniform3f(L("uCamUp"), u.x, u.y, u.z);
    glUniform1f(L("uTanHalfFov"), tanHalf);
    glUniform1f(L("uAspect"), float(winW) / float(winH));
    glUniform1f(L("uPixelAngle"), 2.0f * tanHalf / float(ih));
    glUniform1f(L("uDiskTime"), diskTime);
    glUniform1f(L("uLensingStrength"), s.lensingStrength);
    auto halton = [](unsigned index, unsigned base) {
        float value = 0.0f;
        float fraction = 1.0f / float(base);
        while (index > 0) {
            value += fraction * float(index % base);
            index /= base;
            fraction /= float(base);
        }
        return value;
    };
    const unsigned jitterIndex = frameCounter_ % 8 + 1;
    const float jitterX = s.antiAliasing == AntiAliasingMode::TAA
        ? halton(jitterIndex, 2) - 0.5f : 0.0f;
    const float jitterY = s.antiAliasing == AntiAliasingMode::TAA
        ? halton(jitterIndex, 3) - 0.5f : 0.0f;
    glUniform2f(L("uRayJitter"), jitterX, jitterY);
    glUniform1i(L("uMaxSteps"), qp.maxSteps);
    glUniform1f(L("uStepScale"), qp.stepScale);
    glUniform1i(L("uMaxCrossings"), qp.maxCrossings);
    glUniform1f(L("uEscapeR"), physics::kEscapeRadius);
    glUniform1f(L("uDiskInner"), physics::kIsco);
    glUniform1f(L("uDiskOuter"), physics::kDiskOuter);
    glUniform1f(L("uTempInner"), 7500.0f);
    glUniform1f(L("uTempNorm"), tempNorm_);
    glUniform1f(L("uDiskIntensity"), s.diskIntensity);
    glUniform1f(L("uDopplerStrength"), s.dopplerStrength);
    glUniform1f(L("uArtisticMix"), s.artisticMix);
    glUniform1i(L("uCollectStats"), collect ? 1 : 0);

    glBindImageTexture(0, texHDR_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboStats_);

    if (queryFree) glBeginQuery(GL_TIME_ELAPSED, qRay_[q]);
    glDispatchCompute(GLuint((iw + 15) / 16), GLuint((ih + 15) / 16), 1);
    if (queryFree) glEndQuery(GL_TIME_ELAPSED);

    if (collect) {
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        statsFence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    GLuint sceneTexture = texHDR_;
    if (s.antiAliasing == AntiAliasingMode::TAA) {
        const glm::vec3 cameraPosition = cam.position - blackHolePosition;
        const bool cameraMoved = previousCameraValid_
            && (glm::length(cameraPosition - previousCameraPosition_) > 0.02f
                || glm::dot(f, previousCameraForward_) < 0.99995f);
        const bool modeChanged = previousAAMode_ != AntiAliasingMode::TAA;
        if (cameraMoved || modeChanged
            || std::abs(previousLensingStrength_ - s.lensingStrength) > 1e-4f)
            taaHistoryValid_ = false;

        const int writeIndex = 1 - taaReadIndex_;
        glUseProgram(progTaa_);
        glUniform1i(glGetUniformLocation(progTaa_, "uCurrent"), 0);
        glUniform1i(glGetUniformLocation(progTaa_, "uHistory"), 1);
        glUniform1i(glGetUniformLocation(progTaa_, "uHistoryValid"), taaHistoryValid_ ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texHDR_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texTaa_[taaReadIndex_]);
        glBindImageTexture(0, texTaa_[writeIndex], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(GLuint((iw + 15) / 16), GLuint((ih + 15) / 16), 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        taaReadIndex_ = writeIndex;
        taaHistoryValid_ = true;
        sceneTexture = texTaa_[taaReadIndex_];
        previousCameraPosition_ = cameraPosition;
        previousCameraForward_ = f;
        previousLensingStrength_ = s.lensingStrength;
        previousCameraValid_ = true;
    } else {
        taaHistoryValid_ = false;
        previousAAMode_ = s.antiAliasing;
    }
    previousAAMode_ = s.antiAliasing;

    // ---------- Passe 2: bloom em 1/4 de resolução ----------
    if (s.bloomEnabled) {
        const GLuint gx = GLuint((bloomW_ + 15) / 16), gy = GLuint((bloomH_ + 15) / 16);
        glUseProgram(progDown_);
        glUniform1i(glGetUniformLocation(progDown_, "uSrc"), 0);
        glUniform1f(glGetUniformLocation(progDown_, "uThreshold"), s.bloomThreshold);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glBindImageTexture(0, texBloomA_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        glUseProgram(progBlur_);
        glUniform1i(glGetUniformLocation(progBlur_, "uSrc"), 0);
        const GLint dirLoc = glGetUniformLocation(progBlur_, "uDir");
        for (int it = 0; it < 2; ++it) {
            glUniform2f(dirLoc, 1.0f, 0.0f);
            glBindTexture(GL_TEXTURE_2D, texBloomA_);
            glBindImageTexture(0, texBloomB_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
            glDispatchCompute(gx, gy, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            glUniform2f(dirLoc, 0.0f, 1.0f);
            glBindTexture(GL_TEXTURE_2D, texBloomB_);
            glBindImageTexture(0, texBloomA_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
            glDispatchCompute(gx, gy, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }
    }

    // ---------- Passe 3: tone mapping e apresentação ----------
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, winW, winH);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(progTone_);
    glUniform1i(glGetUniformLocation(progTone_, "uHDR"), 0);
    glUniform1i(glGetUniformLocation(progTone_, "uBloom"), 1);
    glUniform1f(glGetUniformLocation(progTone_, "uExposure"), s.exposure);
    glUniform1f(glGetUniformLocation(progTone_, "uBloomIntensity"), s.bloomIntensity);
    glUniform1i(glGetUniformLocation(progTone_, "uBloomOn"), s.bloomEnabled ? 1 : 0);
    const int msaaSamples = s.antiAliasing == AntiAliasingMode::MSAA2x ? 2
        : s.antiAliasing == AntiAliasingMode::MSAA4x ? 4
        : s.antiAliasing == AntiAliasingMode::MSAA8x ? 8 : 1;
    glUniform1i(glGetUniformLocation(progTone_, "uAAMode"), int(s.antiAliasing));
    glUniform1i(glGetUniformLocation(progTone_, "uMSAASamples"), msaaSamples);
    glUniform2f(glGetUniformLocation(progTone_, "uOutputSize"), float(winW), float(winH));
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texBloomA_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glActiveTexture(GL_TEXTURE0);

    if (queryFree) {
        glQueryCounter(qTotalEnd_[q], GL_TIMESTAMP);
        qPending_[q] = true;
    }
    qIndex_ = (qIndex_ + 1) % kQ;
    ++frameCounter_;
}
