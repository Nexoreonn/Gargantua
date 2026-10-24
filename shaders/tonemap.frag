#version 430 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uHDR;
uniform sampler2D uBloom;
uniform float uExposure;
uniform float uBloomIntensity;
uniform int uBloomOn;
uniform int uAntiAliasing;

vec3 mappedColor(vec2 uv) {
    vec3 hdr = texture(uHDR, uv).rgb;
    if (uBloomOn != 0) hdr += texture(uBloom, uv).rgb * uBloomIntensity;

    vec3 mapped = vec3(1.0) - exp(-hdr * uExposure);
    return pow(max(mapped, vec3(0.0)), vec3(1.0 / 2.2));
}

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 fxaa(vec2 uv) {
    vec2 texel = 1.0 / vec2(textureSize(uHDR, 0));
    vec3 center = mappedColor(uv);
    vec3 northWest = mappedColor(uv + texel * vec2(-1.0, -1.0));
    vec3 northEast = mappedColor(uv + texel * vec2( 1.0, -1.0));
    vec3 southWest = mappedColor(uv + texel * vec2(-1.0,  1.0));
    vec3 southEast = mappedColor(uv + texel * vec2( 1.0,  1.0));
    float lumaCenter = luma(center);
    float lumaNW = luma(northWest);
    float lumaNE = luma(northEast);
    float lumaSW = luma(southWest);
    float lumaSE = luma(southEast);
    float lumaMin = min(lumaCenter, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaCenter, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    if (lumaMax - lumaMin < max(0.0312, lumaMax * 0.125)) return center;

    vec2 direction;
    direction.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    direction.y = (lumaNW + lumaSW) - (lumaNE + lumaSE);
    float reduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.03125, 0.0078125);
    float reciprocalMin = 1.0 / (min(abs(direction.x), abs(direction.y)) + reduce);
    direction = clamp(direction * reciprocalMin, vec2(-8.0), vec2(8.0)) * texel;

    vec3 blendA = 0.5 * (
        mappedColor(uv + direction * (1.0 / 3.0 - 0.5))
        + mappedColor(uv + direction * (2.0 / 3.0 - 0.5)));
    vec3 blendB = blendA * 0.5 + 0.25 * (
        mappedColor(uv + direction * -0.5)
        + mappedColor(uv + direction * 0.5));
    float lumaB = luma(blendB);
    return (lumaB < lumaMin || lumaB > lumaMax) ? blendA : blendB;
}

void main() {
    vec3 mapped = mappedColor(vUV);
    if (uAntiAliasing != 0) mapped = fxaa(vUV);
    FragColor = vec4(mapped, 1.0);
}
