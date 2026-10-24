#include "Physics.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace physics {

float temperatureNormalization() {
    float best = 0.0f;
    for (int i = 1; i <= 4000; ++i) {
        const float x = float(i) / 4000.0f;
        const float f = std::pow(x, 0.75f) * std::pow(std::max(1.0f - std::sqrt(x), 0.0f), 0.25f);
        best = std::max(best, f);
    }
    return best;
}

namespace {
// Mesma equação do shader: d²x/dλ² = -(3/2) r_s h² x / r^5
glm::dvec3 accel(const glm::dvec3& x, double h2) {
    const double r2 = glm::dot(x, x);
    const double r = std::sqrt(r2);
    return -1.5 * double(kRs) * h2 * x / (r2 * r2 * r);
}

enum class Outcome { Captured, Escaped, Undecided };

Outcome trace(double b, double stepScale, glm::dvec3* outDir) {
    glm::dvec3 x(b, 0.0, -1000.0), v(0.0, 0.0, 1.0);
    const double h2 = glm::dot(glm::cross(x, v), glm::cross(x, v));
    for (int i = 0; i < 200000; ++i) {
        const double r = glm::length(x);
        if (r < kRs * 1.002) return Outcome::Captured;
        if (r > 1200.0 && glm::dot(x, v) > 0.0) { *outDir = glm::normalize(v); return Outcome::Escaped; }
        const double dt = stepScale * std::clamp(r - kRs, 0.02, 40.0);
        const glm::dvec3 k1x = v, k1v = accel(x, h2);
        const glm::dvec3 k2x = v + 0.5 * dt * k1v, k2v = accel(x + 0.5 * dt * k1x, h2);
        const glm::dvec3 k3x = v + 0.5 * dt * k2v, k3v = accel(x + 0.5 * dt * k2x, h2);
        const glm::dvec3 k4x = v + dt * k3v,       k4v = accel(x + dt * k3x, h2);
        x += dt / 6.0 * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
        v += dt / 6.0 * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
    }
    return Outcome::Undecided;
}
} // namespace

bool runSelfTest() {
    bool ok = true;
    glm::dvec3 dir;
    std::printf("[selftest] b_critico teorico = %.5f M\n", kCriticalImpact);

    const double captureB[] = { 4.0, 5.0, 5.15 };
    for (double b : captureB) {
        const bool pass = trace(b, 0.02, &dir) == Outcome::Captured;
        std::printf("[selftest] b=%.2f esperado CAPTURA: %s\n", b, pass ? "OK" : "FALHOU");
        ok &= pass;
    }
    const double escapeB[] = { 5.25, 6.0, 10.0 };
    for (double b : escapeB) {
        const bool pass = trace(b, 0.02, &dir) == Outcome::Escaped;
        std::printf("[selftest] b=%.2f esperado ESCAPE: %s\n", b, pass ? "OK" : "FALHOU");
        ok &= pass;
    }

    // Deflexão em campo fraco: alpha ≈ 4M/b + 15πM²/(4b²)
    const double b = 50.0;
    if (trace(b, 0.02, &dir) == Outcome::Escaped) {
        const double alpha = std::atan2(-dir.x, dir.z); // desvio em direção ao BH (-x)
        const double ref = 4.0 / b + 15.0 * 3.14159265358979 / (4.0 * b * b);
        const double relErr = std::abs(alpha - ref) / ref;
        // Tolerância folgada: o raio parte de / termina em r finito (1000-1200 M)
        const bool pass = relErr < 0.03;
        std::printf("[selftest] deflexao b=50: %.5f rad (ref ~%.5f) erro rel %.2f%%: %s\n",
                    alpha, ref, relErr * 100.0, pass ? "OK" : "FALHOU");
        ok &= pass;
    } else {
        std::printf("[selftest] deflexao b=50: raio nao escapou: FALHOU\n");
        ok = false;
    }
    std::printf("[selftest] resultado: %s\n", ok ? "TODOS OK" : "HA FALHAS");
    return ok;
}

} // namespace physics
