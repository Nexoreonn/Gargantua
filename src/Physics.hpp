#pragma once

// Unidades geométricas: G = c = M = 1. Distâncias em unidades de M.
namespace physics {
    constexpr float kM  = 1.0f;
    constexpr float kRs = 2.0f * kM;           // raio de Schwarzschild
    constexpr float kPhotonSphere = 3.0f * kM; // órbitas circulares instáveis de fótons
    constexpr float kIsco = 6.0f * kM;         // órbita circular estável mais interna
    constexpr float kCriticalImpact = 5.196152423f; // 3*sqrt(3) M
    constexpr float kMinCameraRadius = 2.6f * kM;   // > r_s, câmera estática válida
    constexpr float kDiskOuter = 26.0f * kM;
    constexpr float kEscapeRadius = 80.0f * kM;

    // max_x de x^{3/4} (1 - sqrt x)^{1/4}, x = r_in / r (perfil de temperatura)
    float temperatureNormalization();

    // Integra raios na CPU com a mesma equação do shader e verifica:
    // captura/escape em torno de b_c = 3√3 M e o ângulo de deflexão em campo fraco.
    // Retorna true se todos os testes passarem.
    bool runSelfTest();
}
