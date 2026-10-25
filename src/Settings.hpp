#pragma once
#include <algorithm>

// Preset de qualidade numérica do traçado de raios.
struct QualityPreset {
    const char* name;
    int   maxSteps;      // limite de passos RK4 por raio
    float stepScale;     // passo = stepScale * clamp(r - r_s, ...)
    int   maxCrossings;  // nº máximo de cruzamentos com o disco (imagens de ordem n)
};

inline const QualityPreset kQualityPresets[] = {
    { "Rapido",      300, 0.100f, 2 },
    { "Equilibrado", 600, 0.060f, 3 },
    { "Alto",       1200, 0.035f, 4 },
    { "Referencia", 2500, 0.018f, 5 },
};
inline constexpr int kNumQualityPresets = 4;

struct Settings {
    float exposure        = 1.0f;
    bool  bloomEnabled    = true;
    float bloomIntensity  = 0.30f;
    float bloomThreshold  = 1.0f;
    float animSpeed       = 1.0f;
    float blackHoleWanderSpeed = 0.5f;
    float lensingStrength = 1.0f;
    float renderScale     = 0.75f;
    bool  antiAliasing    = true;
    int   quality         = 1;
    float diskIntensity   = 1.0f;
    float dopplerStrength = 1.0f;  // 1 = físico; 0 = desligado (comparação)
    float artisticMix     = 0.35f; // 0 = cor de corpo negro pura; 1 = paleta dourada
    bool  showStats       = true;
    bool  paused          = false;
    bool  vsync           = true;

    const QualityPreset& preset() const {
        return kQualityPresets[std::clamp(quality, 0, kNumQualityPresets - 1)];
    }
};
